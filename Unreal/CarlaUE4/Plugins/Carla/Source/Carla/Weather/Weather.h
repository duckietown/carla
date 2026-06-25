// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"
#include "Carla/Weather/WeatherParameters.h"

#include "Weather.generated.h"

class ASensor;
class ASceneCaptureCamera;

UCLASS(Abstract)
class CARLA_API AWeather : public AActor
{
  GENERATED_BODY()

public:

  AWeather(const FObjectInitializer& ObjectInitializer);

  /// Update the weather parameters and notifies it to the blueprint's event
  UFUNCTION(BlueprintCallable)
  void ApplyWeather(const FWeatherParameters &WeatherParameters);

  /// Notifing the weather to the blueprint's event
  void NotifyWeather(ASensor* Sensor = nullptr);

  /// Update the weather parameters without notifing it to the blueprint's event
  UFUNCTION(BlueprintCallable)
  void SetWeather(const FWeatherParameters &WeatherParameters);

  /// Returns the current WeatherParameters
  UFUNCTION(BlueprintCallable)
  const FWeatherParameters &GetCurrentWeather() const
  {
    return Weather;
  }

  /// Returns whether the day night cycle is active (automatic on/off switch when changin to night mode)
  UFUNCTION(BlueprintCallable)
  const bool &GetDayNightCycle() const
  {
    return DayNightCycle;
  }

  /// Update the day night cycle
  void SetDayNightCycle(const bool &active);

  /// Enable or disable "HDRI mode". While active, this actor (the sky/weather
  /// BP_Sky) is hidden and its directional light, sky light and sky atmosphere
  /// are disabled so that only the HDRIBackdrop lights the scene. Disabling
  /// restores them. Used by the HDRI Python API.
  void SetHDRIMode(bool bEnable);

  /// Whether HDRI mode is currently active on this weather actor.
  bool IsHDRIModeActive() const { return bHDRIModeActive; }

protected:

  UFUNCTION(BlueprintImplementableEvent)
  void RefreshWeather(const FWeatherParameters &WeatherParameters);

private:

  void CheckWeatherPostProcessEffects();

  /// Hide or show this actor and its entire attached / child-actor hierarchy
  /// (sky mesh, sun, sky light, sky atmosphere). Used to turn the regular sky
  /// off and on for HDRI mode.
  void SetSkyHierarchyHidden(bool bHidden);

  bool bHDRIModeActive = false;

  UPROPERTY(VisibleAnywhere)
  FWeatherParameters Weather;

  UMaterial* PrecipitationPostProcessMaterial;

  UMaterial* DustStormPostProcessMaterial;

  TMap<UMaterial*, float> ActiveBlendables;

  UPROPERTY(EditAnywhere, Category = "Weather")
  bool DayNightCycle = true;
};
