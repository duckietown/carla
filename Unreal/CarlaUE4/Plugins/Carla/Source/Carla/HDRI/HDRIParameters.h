// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "HDRIParameters.generated.h"

// -- HDRI --------------------------------------------------------------------
USTRUCT(BlueprintType)
struct CARLA_API FHDRIParameters
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  bool bEnabled = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FString Asset;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = "0.0"))
  float Intensity = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin = "0.0"))
  float Size = 1000.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FVector ProjectionCenter = FVector::ZeroVector;

  /// World-space location (in Unreal coordinates, cm) at which the HDRIBackdrop
  /// actor is placed. Applied directly as the actor's transform location.
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FVector Location = FVector::ZeroVector;
};
