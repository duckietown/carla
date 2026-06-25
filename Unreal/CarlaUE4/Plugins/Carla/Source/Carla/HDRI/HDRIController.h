// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"
#include "Carla/HDRI/HDRIParameters.h"

#include "HDRIController.generated.h"

class UTextureCube;

/// Controls the HDRIBackdrop Blueprint actor used for image-based lighting.
///
/// Only this controller needs to be placed in a map. If the map does not
/// already contain an HDRIBackdrop actor, the controller spawns one from the
/// engine's HDRIBackdrop plugin on first use.
///
/// The HDRIBackdrop ships as a Blueprint-only actor, so we access its exposed
/// properties (Cubemap, Size, Intensity, ProjectionCenter) through UE4's
/// reflection system at runtime.
UCLASS()
class CARLA_API AHDRIController : public AActor
{
  GENERATED_BODY()

public:

  AHDRIController(const FObjectInitializer& ObjectInitializer);

  /// High-level entry point used by the server / Python API.
  ///
  /// Resolves the cubemap named in @a Params (against the default HDRI asset
  /// directory), applies it together with the other parameters to the
  /// HDRIBackdrop, and marks HDRI mode as active.
  ///
  /// Returns false (and changes nothing) if the map has no HDRIBackdrop actor
  /// or the requested cubemap asset could not be loaded. Callers should treat
  /// false as "this map does not support HDRI".
  bool ApplyHDRI(const FHDRIParameters& Params);

  /// Deactivate HDRI mode: hide the HDRIBackdrop so it stops contributing to
  /// the scene. The caller is responsible for restoring the regular sky.
  void DisableHDRI();

  /// Whether HDRI mode is currently active.
  bool IsHDRIActive() const { return bHDRIActive; }

  /// Return the current HDRI state, reading live values back from the backdrop
  /// when one is present.
  FHDRIParameters GetHDRIParameters() const;

  /// Apply all HDRI parameters to the HDRIBackdrop in the scene.
  /// Pass nullptr for CubeMap to leave the current cubemap unchanged.
  UFUNCTION(BlueprintCallable, Category = "HDRI")
  void ApplyHDRIParameters(
      UTextureCube* CubeMap,
      float Size,
      float Intensity,
      FVector ProjectionCenter);

  /// Get the current size of the HDRIBackdrop.
  UFUNCTION(BlueprintCallable, Category = "HDRI")
  float GetSize() const;

  /// Get the current intensity of the HDRIBackdrop.
  UFUNCTION(BlueprintCallable, Category = "HDRI")
  float GetIntensity() const;

  /// Get the current projection center of the HDRIBackdrop.
  UFUNCTION(BlueprintCallable, Category = "HDRI")
  FVector GetProjectionCenter() const;

  /// Get the current cubemap of the HDRIBackdrop.
  UFUNCTION(BlueprintCallable, Category = "HDRI")
  UTextureCube* GetCubeMap() const;

private:

  /// Find and cache an existing HDRIBackdrop actor in the current world.
  /// Does not spawn one. Returns true if a valid backdrop was found or already
  /// cached.
  bool FindHDRIBackdrop();

  /// Spawn an HDRIBackdrop actor from the engine's HDRIBackdrop plugin at the
  /// given world location. Spawning at the target transform guarantees the
  /// placement even if the backdrop's components use Static mobility.
  /// Returns nullptr if the plugin class cannot be loaded or the spawn fails.
  AActor* SpawnHDRIBackdrop(UWorld* World, const FVector& Location);

  /// Make every scene component of the cached backdrop Movable so the actor can
  /// be repositioned at runtime (HDRIBackdrop ships with Static components).
  void MakeBackdropMovable();

  /// Resolve a cubemap by name against the default HDRI asset directory.
  /// Accepts either a bare asset name (e.g. "HDRi_Neutral") or a full object
  /// path starting with '/'. Returns nullptr if the asset cannot be loaded.
  UTextureCube* LoadCubeMapByName(const FString& Name) const;

  /// Helper: set a float property on the cached backdrop by name.
  bool SetFloatProperty(const FName& PropertyName, float Value);

  /// Helper: get a float property from the cached backdrop by name.
  bool GetFloatProperty(const FName& PropertyName, float& OutValue) const;

  /// Helper: set a vector property on the cached backdrop by name.
  bool SetVectorProperty(const FName& PropertyName, const FVector& Value);

  /// Helper: get a vector property from the cached backdrop by name.
  bool GetVectorProperty(const FName& PropertyName, FVector& OutValue) const;

  /// Cached pointer to the HDRIBackdrop Blueprint actor in the scene.
  UPROPERTY()
  AActor* CachedBackdrop;

  /// Whether HDRI mode is currently active.
  UPROPERTY()
  bool bHDRIActive = false;

  /// Name of the cubemap asset last requested through ApplyHDRI (used so that
  /// GetHDRIParameters can report it back; the live float/vector values are
  /// read straight from the backdrop).
  UPROPERTY()
  FString CurrentAsset;
};
