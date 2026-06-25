// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"

#include "HDRIController.generated.h"

class UTextureCube;

/// Controls the HDRIBackdrop Blueprint actor that is placed in the scene.
///
/// The HDRIBackdrop plugin ships as a Blueprint-only actor, so we access its
/// exposed properties (Cubemap, Size, Intensity, ProjectionCenter) through
/// UE4's reflection system at runtime.
UCLASS()
class CARLA_API AHDRIController : public AActor
{
  GENERATED_BODY()

public:

  AHDRIController(const FObjectInitializer& ObjectInitializer);

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

  /// Find and cache the HDRIBackdrop actor in the current world.
  /// Returns true if a valid backdrop was found or already cached.
  bool FindHDRIBackdrop();

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
};
