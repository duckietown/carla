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
  CachedBackdrop = nullptr;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool AHDRIController::ApplyHDRI(const FHDRIParameters& Params)
{
  // Resolve the cubemap by name first. An empty name leaves the current
  // cubemap unchanged; a non-empty name that fails to load is treated as an
  // error so we never apply a half-configured state.
  UTextureCube* CubeMap = nullptr;
  if (!Params.Asset.IsEmpty())
  {
    CubeMap = LoadCubeMapByName(Params.Asset);
    if (CubeMap == nullptr)
    {
      return false;
    }
  }

  // Find an existing backdrop, otherwise spawn one *directly at the requested
  // location*. Spawning at the target transform is what guarantees the
  // placement: the HDRIBackdrop ships with Static-mobility components, so a
  // runtime SetActorLocation on a backdrop spawned at the origin would be a
  // no-op. The spawn transform, however, is always honoured.
  if (!FindHDRIBackdrop())
  {
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
      return false;
    }
    CachedBackdrop = SpawnHDRIBackdrop(World, Params.Location);
    if (CachedBackdrop == nullptr)
    {
      return false;
    }
  }

  // Make sure the backdrop is visible again in case it was hidden by a
  // previous DisableHDRI() call.
  CachedBackdrop->SetActorHiddenInGame(false);

  // Allow runtime repositioning (e.g. an already-spawned or editor-placed
  // backdrop) by forcing the components Movable, then place the actor.
  MakeBackdropMovable();
  CachedBackdrop->SetActorLocation(Params.Location);

  ApplyHDRIParameters(
      CubeMap, Params.Size, Params.Intensity, Params.ProjectionCenter);

  // Re-assert the location after ApplyHDRIParameters (which re-runs the
  // construction script) so the transform is never clobbered.
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
  if (CachedBackdrop == nullptr)
  {
    return;
  }

  TArray<USceneComponent*> SceneComponents;
  CachedBackdrop->GetComponents<USceneComponent>(SceneComponents);
  for (USceneComponent* Component : SceneComponents)
  {
    if (Component != nullptr &&
        Component->Mobility != EComponentMobility::Movable)
    {
      Component->SetMobility(EComponentMobility::Movable);
    }
  }
}

void AHDRIController::DisableHDRI()
{
  bHDRIActive = false;
  if (FindHDRIBackdrop())
  {
    // Hide the backdrop so it stops contributing to the scene; the caller
    // restores the regular sky/weather actor.
    CachedBackdrop->SetActorHiddenInGame(true);
  }
  UE_LOG(LogCarla, Log, TEXT("[HDRIController] HDRI mode disabled"));
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
  // Default directory that ships the HDRI cubemaps. Names are resolved as
  // "/Game/Carla/Static/HDRi/<Name>.<Name>". A name that already starts with
  // '/' is treated as a full object path and used verbatim.
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
        TEXT("[HDRIController] Could not load cubemap '%s' (resolved path '%s')"),
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

  // None present. Spawning is handled by the caller (ApplyHDRI), so that the
  // backdrop can be spawned at the requested transform.
  CachedBackdrop = nullptr;
  return false;
}

AActor* AHDRIController::SpawnHDRIBackdrop(UWorld* World, const FVector& Location)
{
  // Generated Blueprint class shipped by the engine's HDRIBackdrop plugin.
  static const TCHAR* BackdropClassPath =
      TEXT("/HDRIBackdrop/Blueprints/HDRIBackdrop.HDRIBackdrop_C");

  UClass* BackdropClass = LoadClass<AActor>(nullptr, BackdropClassPath);
  if (BackdropClass == nullptr)
  {
    UE_LOG(LogCarla, Error,
        TEXT("[HDRIController] Could not load HDRIBackdrop class '%s'. "
             "Is the HDRIBackdrop plugin enabled?"),
        BackdropClassPath);
    return nullptr;
  }

  // Spawn directly at the requested transform so the placement holds even if
  // the backdrop's components use Static mobility.
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  AActor* Spawned = World->SpawnActor<AActor>(
      BackdropClass, FTransform(Location), SpawnParams);

  if (Spawned == nullptr)
  {
    UE_LOG(LogCarla, Error,
        TEXT("[HDRIController] Failed to spawn HDRIBackdrop actor"));
    return nullptr;
  }

  UE_LOG(LogCarla, Log,
      TEXT("[HDRIController] Spawned HDRIBackdrop actor at (%.1f, %.1f, %.1f) "
           "(none present in map)"),
      Location.X, Location.Y, Location.Z);
  return Spawned;
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
