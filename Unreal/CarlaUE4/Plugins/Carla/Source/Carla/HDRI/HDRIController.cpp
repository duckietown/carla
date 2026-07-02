// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/HDRI/HDRIController.h"

#include "Engine/TextureCube.h"
#include "Engine/SkyLight.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AHDRIController::AHDRIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = false;
  RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(
      this, TEXT("RootComponent"));
}

bool AHDRIController::ApplyHDRI(const FString& PresetName)
{
  const FHDRIPreset* Preset = nullptr;
  for (const FHDRIPreset& Candidate : Presets)
  {
    if (Candidate.Name.Equals(PresetName, ESearchCase::IgnoreCase))
    {
      Preset = &Candidate;
      break;
    }
  }

  if (Preset == nullptr)
  {
    UE_LOG(LogCarla, Warning,
        TEXT("[HDRIController] HDRI preset '%s' not found in this map."),
        *PresetName);
    return false;
  }

  const FVector Location = GetActorLocation();
  if (!FindHDRIBackdrop())
  {
    CachedBackdrop = SpawnHDRIBackdrop(Location);
    if (CachedBackdrop == nullptr)
    {
      return false;
    }
  }

  CachedBackdrop->SetActorHiddenInGame(false);
  MakeBackdropMovable();
  CachedBackdrop->SetActorLocation(Location);

  ApplyHDRIParameters(Preset->Cubemap, Preset->Size, Preset->Intensity,
                      Preset->ProjectionCenter);

  CachedBackdrop->SetActorLocation(Location);

  ScheduleSkyLightRecapture(Preset->Cubemap);

  ApplySunOverride(Preset->SunIntensity, Preset->SunTemperature);

  bHDRIActive = true;
  return true;
}

TArray<FString> AHDRIController::GetPresetNames() const
{
  TArray<FString> Names;
  Names.Reserve(Presets.Num());
  for (const FHDRIPreset& Preset : Presets)
  {
    Names.Add(Preset.Name);
  }
  return Names;
}

void AHDRIController::MakeBackdropMovable()
{
  TArray<USceneComponent*> SceneComponents;
  CachedBackdrop->GetComponents<USceneComponent>(SceneComponents);
  for (USceneComponent* Component : SceneComponents)
  {
    Component->SetMobility(EComponentMobility::Movable);
  }
}

void AHDRIController::DisableHDRI()
{
  bHDRIActive = false;
  if (FindHDRIBackdrop())
  {
    CachedBackdrop->SetActorHiddenInGame(true);
  }

  RestoreSunOverride();

  ScheduleSkyLightRecapture(nullptr);
}

void AHDRIController::ScheduleSkyLightRecapture(UTextureCube* WaitForCubemap)
{
  PendingRecaptureCubemap = WaitForCubemap;
  RecaptureFence.BeginFence();

  GetWorldTimerManager().SetTimer(
      RecaptureTimerHandle, this, &AHDRIController::TryRecaptureWhenReady,
      0.05f, true);
}

void AHDRIController::TryRecaptureWhenReady()
{
  if (PendingRecaptureCubemap != nullptr &&
      !PendingRecaptureCubemap->IsFullyStreamedIn())
  {
    return;
  }

  if (!RecaptureFence.IsFenceComplete())
  {
    return;
  }

  RecaptureSkyLight();

  GetWorldTimerManager().ClearTimer(RecaptureTimerHandle);
}

void AHDRIController::RecaptureSkyLight()
{
  TArray<AActor*> SkyLights;
  UGameplayStatics::GetAllActorsOfClass(
      GetWorld(), ASkyLight::StaticClass(), SkyLights);
  for (AActor* Actor : SkyLights)
  {
    CastChecked<ASkyLight>(Actor)->GetLightComponent()->SetCaptureIsDirty();
  }

  USkyLightComponent::UpdateSkyCaptureContents(GetWorld());
}

void AHDRIController::ApplyHDRIParameters(
    UTextureCube* CubeMap,
    float Size,
    float Intensity,
    FVector ProjectionCenter)
{
  if (!FindHDRIBackdrop())
  {
    return;
  }

  SetFloatProperty(TEXT("Size"), Size);
  SetFloatProperty(TEXT("Intensity"), Intensity);
  SetVectorProperty(TEXT("ProjectionCenter"), ProjectionCenter);

  if (CubeMap != nullptr)
  {
    if (FObjectProperty* ObjProp = CastField<FObjectProperty>(
            CachedBackdrop->GetClass()->FindPropertyByName(TEXT("Cubemap"))))
    {
      ObjProp->SetObjectPropertyValue(
          ObjProp->ContainerPtrToValuePtr<void>(CachedBackdrop), CubeMap);
    }
  }

  CachedBackdrop->RerunConstructionScripts();
}

bool AHDRIController::FindHDRIBackdrop()
{
  if (IsValid(CachedBackdrop))
  {
    return true;
  }

  TArray<AActor*> AllActors;
  UGameplayStatics::GetAllActorsOfClass(
      GetWorld(), AActor::StaticClass(), AllActors);

  for (AActor* Actor : AllActors)
  {
    if (Actor->GetClass()->GetName().Contains(TEXT("HDRIBackdrop")))
    {
      CachedBackdrop = Actor;
      return true;
    }
  }

  CachedBackdrop = nullptr;
  return false;
}

AActor* AHDRIController::SpawnHDRIBackdrop(const FVector& Location)
{
  static const TCHAR* BackdropClassPath =
      TEXT("/HDRIBackdrop/Blueprints/HDRIBackdrop.HDRIBackdrop_C");

  UClass* BackdropClass = LoadClass<AActor>(nullptr, BackdropClassPath);
  if (BackdropClass == nullptr)
  {
    UE_LOG(LogCarla, Error,
        TEXT("[HDRIController] Could not load HDRIBackdrop class (is the "
             "HDRIBackdrop plugin enabled?)"));
    return nullptr;
  }

  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  return GetWorld()->SpawnActor<AActor>(
      BackdropClass, FTransform(Location), SpawnParams);
}

bool AHDRIController::SetFloatProperty(const FName& PropertyName, float Value)
{
  if (FFloatProperty* FloatProp = CastField<FFloatProperty>(
          CachedBackdrop->GetClass()->FindPropertyByName(PropertyName)))
  {
    FloatProp->SetPropertyValue(
        FloatProp->ContainerPtrToValuePtr<void>(CachedBackdrop), Value);
    return true;
  }
  return false;
}

bool AHDRIController::SetVectorProperty(
    const FName& PropertyName, const FVector& Value)
{
  if (FStructProperty* StructProp = CastField<FStructProperty>(
          CachedBackdrop->GetClass()->FindPropertyByName(PropertyName)))
  {
    if (StructProp->Struct == TBaseStructure<FVector>::Get())
    {
      *StructProp->ContainerPtrToValuePtr<FVector>(CachedBackdrop) = Value;
      return true;
    }
  }
  return false;
}

UDirectionalLightComponent* AHDRIController::FindSunLight()
{
  TArray<AActor*> AllActors;
  UGameplayStatics::GetAllActorsOfClass(
      GetWorld(), AActor::StaticClass(), AllActors);

  for (AActor* Actor : AllActors)
  {
    if (!Actor->GetName().StartsWith(TEXT("BP_Sky")) &&
        !Actor->GetClass()->GetName().StartsWith(TEXT("BP_Sky")))
    {
      continue;
    }

    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors);
    for (AActor* Attached : AttachedActors)
    {
      if (UDirectionalLightComponent* Light =
              Attached->FindComponentByClass<UDirectionalLightComponent>())
      {
        return Light;
      }
    }

    UE_LOG(LogCarla, Warning,
        TEXT("[HDRIController] BP_Sky actor '%s' has no attached DirectionalLight."),
        *Actor->GetName());
  }

  return nullptr;
}

void AHDRIController::ApplySunOverride(float SunIntensity, float SunTemperature)
{
  if (SunIntensity < 0.0f && SunTemperature < 0.0f)
  {
    return;
  }

  UDirectionalLightComponent* SunLight = FindSunLight();
  if (SunLight == nullptr)
  {
    UE_LOG(LogCarla, Warning,
        TEXT("[HDRIController] Could not find a 'BP_Sky' actor with a "
             "DirectionalLight to override."));
    return;
  }

  if (!bSunOverridden)
  {
    SavedSunIntensity = SunLight->Intensity;
    SavedSunTemperature = SunLight->Temperature;
    bSavedUseTemperature = SunLight->bUseTemperature;
    CachedSunLight = SunLight;
    bSunOverridden = true;
  }

  if (SunIntensity >= 0.0f)
  {
    SunLight->SetIntensity(SunIntensity);
  }
  if (SunTemperature >= 0.0f)
  {
    SunLight->bUseTemperature = true;
    SunLight->SetTemperature(SunTemperature);
    SunLight->MarkRenderStateDirty();
  }
}

void AHDRIController::RestoreSunOverride()
{
  if (!bSunOverridden)
  {
    return;
  }

  if (IsValid(CachedSunLight))
  {
    CachedSunLight->SetIntensity(SavedSunIntensity);
    CachedSunLight->bUseTemperature = bSavedUseTemperature;
    CachedSunLight->SetTemperature(SavedSunTemperature);
    CachedSunLight->MarkRenderStateDirty();
  }

  bSunOverridden = false;
  CachedSunLight = nullptr;
}
