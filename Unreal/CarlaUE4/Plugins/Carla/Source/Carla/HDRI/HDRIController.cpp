// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/HDRI/HDRIController.h"

#include "Engine/TextureCube.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AHDRIController::AHDRIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = false;
  RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(
      this, TEXT("RootComponent"));
}

bool AHDRIController::ApplyHDRI(const FHDRIParameters& Params)
{
  UTextureCube* CubeMap = nullptr;
  if (!Params.Asset.IsEmpty())
  {
    CubeMap = LoadCubeMapByName(Params.Asset);
    if (CubeMap == nullptr)
    {
      return false;
    }
  }

  if (!FindHDRIBackdrop())
  {
    CachedBackdrop = SpawnHDRIBackdrop(Params.Location);
    if (CachedBackdrop == nullptr)
    {
      return false;
    }
  }

  CachedBackdrop->SetActorHiddenInGame(false);
  MakeBackdropMovable();
  CachedBackdrop->SetActorLocation(Params.Location);

  ApplyHDRIParameters(
      CubeMap, Params.Size, Params.Intensity, Params.ProjectionCenter);

  CachedBackdrop->SetActorLocation(Params.Location);

  if (!Params.Asset.IsEmpty())
  {
    CurrentAsset = Params.Asset;
  }
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
}

FHDRIParameters AHDRIController::GetHDRIParameters() const
{
  FHDRIParameters Params;
  Params.bEnabled = bHDRIActive;
  Params.Asset = CurrentAsset;
  if (CachedBackdrop != nullptr)
  {
    Params.Size = GetSize();
    Params.Intensity = GetIntensity();
    Params.ProjectionCenter = GetProjectionCenter();
    Params.Location = CachedBackdrop->GetActorLocation();
  }
  return Params;
}

UTextureCube* AHDRIController::LoadCubeMapByName(const FString& Name) const
{
  static const TCHAR* BaseDir = TEXT("/Game/Carla/Static/HDRi/");

  FString ObjectPath = Name;
  if (!Name.StartsWith(TEXT("/")))
  {
    ObjectPath = FString::Printf(TEXT("%s%s.%s"), BaseDir, *Name, *Name);
  }

  UTextureCube* CubeMap = LoadObject<UTextureCube>(nullptr, *ObjectPath);
  if (CubeMap == nullptr)
  {
    UE_LOG(LogCarla, Error,
        TEXT("[HDRIController] Could not load cubemap '%s' (resolved '%s')"),
        *Name, *ObjectPath);
  }
  return CubeMap;
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

float AHDRIController::GetSize() const
{
  float Value = 0.0f;
  GetFloatProperty(TEXT("Size"), Value);
  return Value;
}

float AHDRIController::GetIntensity() const
{
  float Value = 0.0f;
  GetFloatProperty(TEXT("Intensity"), Value);
  return Value;
}

FVector AHDRIController::GetProjectionCenter() const
{
  FVector Value = FVector::ZeroVector;
  GetVectorProperty(TEXT("ProjectionCenter"), Value);
  return Value;
}

UTextureCube* AHDRIController::GetCubeMap() const
{
  if (CachedBackdrop == nullptr)
  {
    return nullptr;
  }

  if (FObjectProperty* ObjProp = CastField<FObjectProperty>(
          CachedBackdrop->GetClass()->FindPropertyByName(TEXT("Cubemap"))))
  {
    return Cast<UTextureCube>(ObjProp->GetObjectPropertyValue(
        ObjProp->ContainerPtrToValuePtr<void>(CachedBackdrop)));
  }
  return nullptr;
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

bool AHDRIController::GetFloatProperty(
    const FName& PropertyName, float& OutValue) const
{
  if (CachedBackdrop == nullptr)
  {
    return false;
  }
  if (FFloatProperty* FloatProp = CastField<FFloatProperty>(
          CachedBackdrop->GetClass()->FindPropertyByName(PropertyName)))
  {
    OutValue = FloatProp->GetPropertyValue(
        FloatProp->ContainerPtrToValuePtr<void>(CachedBackdrop));
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

bool AHDRIController::GetVectorProperty(
    const FName& PropertyName, FVector& OutValue) const
{
  if (CachedBackdrop == nullptr)
  {
    return false;
  }
  if (FStructProperty* StructProp = CastField<FStructProperty>(
          CachedBackdrop->GetClass()->FindPropertyByName(PropertyName)))
  {
    if (StructProp->Struct == TBaseStructure<FVector>::Get())
    {
      OutValue = *StructProp->ContainerPtrToValuePtr<FVector>(CachedBackdrop);
      return true;
    }
  }
  return false;
}
