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

  UPROPERTY()
  AActor* CachedBackdrop = nullptr;

  FTimerHandle RecaptureTimerHandle;

  UPROPERTY()
  UTextureCube* PendingRecaptureCubemap = nullptr;

  FRenderCommandFence RecaptureFence;

  UPROPERTY()
  bool bHDRIActive = false;


  UPROPERTY(EditAnywhere, Category = "HDRI")
  TArray<FHDRIPreset> Presets;
};
