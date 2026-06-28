// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/HDRI/HDRIController.h"

#include "Engine/TextureCube.h"
#include "Components/SceneComponent.h"
// #include "Components/LightComponentBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AHDRIController::AHDRIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = false;
  RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(
      this, TEXT("RootComponent"));
}

bool AHDRIController::ApplyHDRIByName(const FString& PresetName)
{
  for (const FHDRIPreset& Preset : Presets)
  {
    if (Preset.Name.Equals(PresetName, ESearchCase::IgnoreCase))
    {
      const FString AssetName =
          Preset.Cubemap != nullptr ? Preset.Cubemap->GetName() : FString();
      return ApplyHDRI(Preset.Cubemap, Preset.Size, Preset.Intensity,
                       Preset.ProjectionCenter, GetActorLocation(),
                       AssetName);
    }
  }
  UE_LOG(LogCarla, Warning,
      TEXT("[HDRIController] HDRI preset '%s' not found in this map."),
      *PresetName);
  return false;
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

bool AHDRIController::ApplyHDRI(
    UTextureCube* CubeMap, float Size, float Intensity,
    FVector ProjectionCenter, FVector Location, const FString& AssetName)
{
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

  ApplyHDRIParameters(CubeMap, Size, Intensity, ProjectionCenter);

  CachedBackdrop->SetActorLocation(Location);

  if (!AssetName.IsEmpty())
  {
    CurrentAsset = AssetName;
  }

  // SetSkyHidden(true);

  bHDRIActive = true;
  return true;
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

  // Restore Carla's sky and its lights.
  // SetSkyHidden(false);
}

// AActor* AHDRIController::FindSkyActor()
// {
//   if (IsValid(CachedSkyActor))
//   {
//     return CachedSkyActor;
//   }
//
//   TArray<AActor*> Actors;
//   UGameplayStatics::GetAllActorsOfClass(
//       GetWorld(), AActor::StaticClass(), Actors);
//   for (AActor* Actor : Actors)
//   {
//     if (Actor != nullptr && Actor->GetClass()->GetName().Equals(TEXT("BP_Sky_C")))
//     {
//       CachedSkyActor = Actor;
//       break;
//     }
//   }
//   return CachedSkyActor;
// }
//
// void AHDRIController::SetSkyHidden(bool bHidden)
// {
//   AActor* SkyActor = FindSkyActor();
//   if (SkyActor == nullptr)
//   {
//     UE_LOG(LogCarla, Warning,
//         TEXT("[HDRIController] BP_Sky_C not found; cannot toggle Carla sky."));
//     return;
//   }
//
//   SkyActor->SetActorHiddenInGame(bHidden);
//
//   TArray<AActor*> SkyActors;
//   SkyActors.Add(SkyActor);
//   SkyActor->GetAttachedActors(SkyActors, /*bResetArray=*/false);
//
//   for (AActor* Actor : SkyActors)
//   {
//     TArray<ULightComponentBase*> Lights;
//     Actor->GetComponents<ULightComponentBase>(Lights);
//     for (ULightComponentBase* Light : Lights)
//     {
//       Light->SetVisibility(!bHidden, true);
//     }
//   }
// }

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
