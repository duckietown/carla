// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/HDRI/HDRIController.h"

#include "Engine/TextureCube.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AHDRIController::AHDRIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = false;
  RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(
      this, TEXT("RootComponent"));
  CachedBackdrop = nullptr;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

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
    // Cubemap is an object (UTextureCube*) property on the Blueprint.
    UClass* BackdropClass = CachedBackdrop->GetClass();
    FProperty* Prop = BackdropClass->FindPropertyByName(TEXT("Cubemap"));
    if (Prop != nullptr)
    {
      FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop);
      if (ObjProp != nullptr)
      {
        ObjProp->SetObjectPropertyValue(
            Prop->ContainerPtrToValuePtr<void>(CachedBackdrop), CubeMap);
      }
      else
      {
        UE_LOG(LogCarla, Warning,
            TEXT("[HDRIController] 'Cubemap' property is not an object property"));
      }
    }
    else
    {
      UE_LOG(LogCarla, Warning,
          TEXT("[HDRIController] Could not find 'Cubemap' property on HDRIBackdrop"));
    }
  }

  // Force the backdrop to re-run its construction script so that the
  // material instance and skylight pick up the new values.
  CachedBackdrop->RerunConstructionScripts();

  UE_LOG(LogCarla, Log,
      TEXT("[HDRIController] Applied HDRI parameters — Size: %.1f, "
           "Intensity: %.2f, Projection: (%.1f, %.1f, %.1f)"),
      Size, Intensity,
      ProjectionCenter.X, ProjectionCenter.Y, ProjectionCenter.Z);
}

float AHDRIController::GetSize() const
{
  float Value = 0.0f;
  if (CachedBackdrop != nullptr)
  {
    GetFloatProperty(TEXT("Size"), Value);
  }
  return Value;
}

float AHDRIController::GetIntensity() const
{
  float Value = 0.0f;
  if (CachedBackdrop != nullptr)
  {
    GetFloatProperty(TEXT("Intensity"), Value);
  }
  return Value;
}

FVector AHDRIController::GetProjectionCenter() const
{
  FVector Value = FVector::ZeroVector;
  if (CachedBackdrop != nullptr)
  {
    GetVectorProperty(TEXT("ProjectionCenter"), Value);
  }
  return Value;
}

UTextureCube* AHDRIController::GetCubeMap() const
{
  if (CachedBackdrop == nullptr)
  {
    return nullptr;
  }

  UClass* BackdropClass = CachedBackdrop->GetClass();
  FProperty* Prop = BackdropClass->FindPropertyByName(TEXT("Cubemap"));
  if (Prop == nullptr)
  {
    return nullptr;
  }

  FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop);
  if (ObjProp == nullptr)
  {
    return nullptr;
  }

  UObject* Obj = ObjProp->GetObjectPropertyValue(
      Prop->ContainerPtrToValuePtr<void>(CachedBackdrop));
  return Cast<UTextureCube>(Obj);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool AHDRIController::FindHDRIBackdrop()
{
  if (CachedBackdrop != nullptr && IsValid(CachedBackdrop))
  {
    return true;
  }

  UWorld* World = GetWorld();
  if (World == nullptr)
  {
    UE_LOG(LogCarla, Error,
        TEXT("[HDRIController] No world available"));
    return false;
  }

  // The HDRIBackdrop is a Blueprint actor whose generated class is named
  // "HDRIBackdrop_C". We iterate over all actors and match by class name.
  TArray<AActor*> AllActors;
  UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

  for (AActor* Actor : AllActors)
  {
    if (Actor != nullptr &&
        Actor->GetClass()->GetName().Contains(TEXT("HDRIBackdrop")))
    {
      CachedBackdrop = Actor;
      UE_LOG(LogCarla, Log,
          TEXT("[HDRIController] Found HDRIBackdrop actor: %s (class: %s)"),
          *Actor->GetName(), *Actor->GetClass()->GetName());
      return true;
    }
  }

  UE_LOG(LogCarla, Error,
      TEXT("[HDRIController] No HDRIBackdrop actor found in the scene"));
  CachedBackdrop = nullptr;
  return false;
}

bool AHDRIController::SetFloatProperty(const FName& PropertyName, float Value)
{
  check(CachedBackdrop != nullptr);

  UClass* BackdropClass = CachedBackdrop->GetClass();
  FProperty* Prop = BackdropClass->FindPropertyByName(PropertyName);
  if (Prop == nullptr)
  {
    UE_LOG(LogCarla, Warning,
        TEXT("[HDRIController] Could not find property '%s' on HDRIBackdrop"),
        *PropertyName.ToString());
    return false;
  }

  FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop);
  if (FloatProp != nullptr)
  {
    FloatProp->SetPropertyValue(
        Prop->ContainerPtrToValuePtr<void>(CachedBackdrop), Value);
    return true;
  }

  UE_LOG(LogCarla, Warning,
      TEXT("[HDRIController] Property '%s' is not a float"),
      *PropertyName.ToString());
  return false;
}

bool AHDRIController::GetFloatProperty(
    const FName& PropertyName, float& OutValue) const
{
  check(CachedBackdrop != nullptr);

  UClass* BackdropClass = CachedBackdrop->GetClass();
  FProperty* Prop = BackdropClass->FindPropertyByName(PropertyName);
  if (Prop == nullptr)
  {
    return false;
  }

  FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop);
  if (FloatProp != nullptr)
  {
    OutValue = FloatProp->GetPropertyValue(
        Prop->ContainerPtrToValuePtr<void>(CachedBackdrop));
    return true;
  }

  return false;
}

bool AHDRIController::SetVectorProperty(
    const FName& PropertyName, const FVector& Value)
{
  check(CachedBackdrop != nullptr);

  UClass* BackdropClass = CachedBackdrop->GetClass();
  FProperty* Prop = BackdropClass->FindPropertyByName(PropertyName);
  if (Prop == nullptr)
  {
    UE_LOG(LogCarla, Warning,
        TEXT("[HDRIController] Could not find property '%s' on HDRIBackdrop"),
        *PropertyName.ToString());
    return false;
  }

  FStructProperty* StructProp = CastField<FStructProperty>(Prop);
  if (StructProp != nullptr &&
      StructProp->Struct == TBaseStructure<FVector>::Get())
  {
    *StructProp->ContainerPtrToValuePtr<FVector>(CachedBackdrop) = Value;
    return true;
  }

  UE_LOG(LogCarla, Warning,
      TEXT("[HDRIController] Property '%s' is not an FVector"),
      *PropertyName.ToString());
  return false;
}

bool AHDRIController::GetVectorProperty(
    const FName& PropertyName, FVector& OutValue) const
{
  check(CachedBackdrop != nullptr);

  UClass* BackdropClass = CachedBackdrop->GetClass();
  FProperty* Prop = BackdropClass->FindPropertyByName(PropertyName);
  if (Prop == nullptr)
  {
    return false;
  }

  FStructProperty* StructProp = CastField<FStructProperty>(Prop);
  if (StructProp != nullptr &&
      StructProp->Struct == TBaseStructure<FVector>::Get())
  {
    OutValue = *StructProp->ContainerPtrToValuePtr<FVector>(CachedBackdrop);
    return true;
  }

  return false;
}
