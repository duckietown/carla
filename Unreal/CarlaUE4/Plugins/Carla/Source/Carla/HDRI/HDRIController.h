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

UCLASS()
class CARLA_API AHDRIController : public AActor
{
  GENERATED_BODY()

public:

  AHDRIController(const FObjectInitializer& ObjectInitializer);

  bool ApplyHDRI(const FHDRIParameters& Params);

  void DisableHDRI();

  bool IsHDRIActive() const { return bHDRIActive; }

  FHDRIParameters GetHDRIParameters() const;

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  void ApplyHDRIParameters(
      UTextureCube* CubeMap,
      float Size,
      float Intensity,
      FVector ProjectionCenter);

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  float GetSize() const;

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  float GetIntensity() const;

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  FVector GetProjectionCenter() const;

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  UTextureCube* GetCubeMap() const;

private:

  bool FindHDRIBackdrop();

  AActor* SpawnHDRIBackdrop(const FVector& Location);

  AActor* FindSkyActor();

  void SetSkyHidden(bool bHidden);

  void MakeBackdropMovable();

  UTextureCube* LoadCubeMapByName(const FString& Name) const;

  bool SetFloatProperty(const FName& PropertyName, float Value);
  bool GetFloatProperty(const FName& PropertyName, float& OutValue) const;
  bool SetVectorProperty(const FName& PropertyName, const FVector& Value);
  bool GetVectorProperty(const FName& PropertyName, FVector& OutValue) const;

  UPROPERTY()
  AActor* CachedBackdrop = nullptr;

  UPROPERTY()
  AActor* CachedSkyActor = nullptr;

  UPROPERTY()
  bool bHDRIActive = false;

  UPROPERTY()
  FString CurrentAsset;
};
