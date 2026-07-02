// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"
#include "RenderCommandFence.h"

#include "HDRIController.generated.h"

class UTextureCube;
class UDirectionalLightComponent;

USTRUCT(BlueprintType)
struct FHDRIPreset
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI")
  FString Name;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI")
  UTextureCube* Cubemap = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI", meta = (ClampMin = "0.0"))
  float Intensity = 100.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI", meta = (ClampMin = "0.0"))
  float Size = 200.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI")
  FVector ProjectionCenter = FVector(0.0f, 0.0f, 1000.0f);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI")
  float SunIntensity = -1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HDRI")
  float SunTemperature = -1.0f;
};

UCLASS()
class CARLA_API AHDRIController : public AActor
{
  GENERATED_BODY()

public:

  AHDRIController(const FObjectInitializer& ObjectInitializer);

  bool ApplyHDRI(const FString& PresetName);

  TArray<FString> GetPresetNames() const;

  void DisableHDRI();

  bool IsHDRIActive() const { return bHDRIActive; }

  UFUNCTION(BlueprintCallable, Category = "HDRI")
  void ApplyHDRIParameters(
      UTextureCube* CubeMap,
      float Size,
      float Intensity,
      FVector ProjectionCenter);

private:

  bool FindHDRIBackdrop();

  AActor* SpawnHDRIBackdrop(const FVector& Location);

  void MakeBackdropMovable();

  void RecaptureSkyLight();

  void ScheduleSkyLightRecapture(UTextureCube* WaitForCubemap);

  void TryRecaptureWhenReady();

  bool SetFloatProperty(const FName& PropertyName, float Value);
  bool SetVectorProperty(const FName& PropertyName, const FVector& Value);

  // Finds the DirectionalLight owned by the "BP_Sky" actor in the level.
  UDirectionalLightComponent* FindSunLight();

  // Overrides the sun light intensity/temperature for the given preset,
  // caching the original values so they can be restored later. Values of -1
  // are treated as "not set" and leave the corresponding property untouched.
  void ApplySunOverride(float SunIntensity, float SunTemperature);

  // Restores the sun light values cached by ApplySunOverride (no-op if the
  // sun was never overridden).
  void RestoreSunOverride();

  UPROPERTY()
  AActor* CachedBackdrop = nullptr;

  FTimerHandle RecaptureTimerHandle;

  UPROPERTY()
  UTextureCube* PendingRecaptureCubemap = nullptr;

  FRenderCommandFence RecaptureFence;

  UPROPERTY()
  bool bHDRIActive = false;

  // Cached sun light and its original values while an HDRI preset overrides
  // them. bSunOverridden guards against capturing already-overridden values.
  UPROPERTY()
  UDirectionalLightComponent* CachedSunLight = nullptr;

  bool bSunOverridden = false;
  float SavedSunIntensity = 0.0f;
  float SavedSunTemperature = 0.0f;
  bool bSavedUseTemperature = false;

  UPROPERTY(EditAnywhere, Category = "HDRI")
  TArray<FHDRIPreset> Presets;
};
