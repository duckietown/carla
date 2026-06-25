// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "Carla/Weather/Weather.h"
#include "Carla/Sensor/SceneCaptureCamera.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/ChildActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ConstructorHelpers.h"
#include "Carla/Game/CarlaStatics.h"
#include "Carla/Recorder/CarlaRecorder.h"
#include "Carla/Recorder/CarlaRecorderWeather.h"

AWeather::AWeather(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrecipitationPostProcessMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(
        TEXT("Material'/Game/Carla/Static/GenericMaterials/00_MastersOpt/Screen_posProcess/M_screenDrops.M_screenDrops'")).Object;

    DustStormPostProcessMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(
        TEXT("Material'/Game/Carla/Static/GenericMaterials/00_MastersOpt/Screen_posProcess/M_screenDust_wind.M_screenDust_wind'")).Object;

    PrimaryActorTick.bCanEverTick = false;
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));
}

void AWeather::CheckWeatherPostProcessEffects()
{
    if (Weather.Precipitation > 0.0f)
        ActiveBlendables.Add(MakeTuple(PrecipitationPostProcessMaterial, Weather.Precipitation / 100.0f));
    else
        ActiveBlendables.Remove(PrecipitationPostProcessMaterial);

    if (Weather.DustStorm > 0.0f)
        ActiveBlendables.Add(MakeTuple(DustStormPostProcessMaterial, Weather.DustStorm / 100.0f));
    else
        ActiveBlendables.Remove(DustStormPostProcessMaterial);

    TArray<AActor*> SensorActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASceneCaptureCamera::StaticClass(), SensorActors);
    for (AActor* SensorActor : SensorActors)
    {
        ASceneCaptureCamera* Sensor = Cast<ASceneCaptureCamera>(SensorActor);
        for (auto& ActiveBlendable : ActiveBlendables)
            Sensor->GetCaptureComponent2D()->PostProcessSettings.AddBlendable(ActiveBlendable.Key, ActiveBlendable.Value);
    }
}

void AWeather::ApplyWeather(const FWeatherParameters& InWeather)
{
    SetWeather(InWeather);
    CheckWeatherPostProcessEffects();

#ifdef CARLA_WEATHER_EXTRA_LOG
    UE_LOG(LogCarla, Log, TEXT("Changing weather:"));
    UE_LOG(LogCarla, Log, TEXT("  - Cloudiness = %.2f"), Weather.Cloudiness);
    UE_LOG(LogCarla, Log, TEXT("  - Precipitation = %.2f"), Weather.Precipitation);
    UE_LOG(LogCarla, Log, TEXT("  - PrecipitationDeposits = %.2f"), Weather.PrecipitationDeposits);
    UE_LOG(LogCarla, Log, TEXT("  - WindIntensity = %.2f"), Weather.WindIntensity);
    UE_LOG(LogCarla, Log, TEXT("  - SunAzimuthAngle = %.2f"), Weather.SunAzimuthAngle);
    UE_LOG(LogCarla, Log, TEXT("  - SunAltitudeAngle = %.2f"), Weather.SunAltitudeAngle);
    UE_LOG(LogCarla, Log, TEXT("  - FogDensity = %.2f"), Weather.FogDensity);
    UE_LOG(LogCarla, Log, TEXT("  - FogDistance = %.2f"), Weather.FogDistance);
    UE_LOG(LogCarla, Log, TEXT("  - FogFalloff = %.2f"), Weather.FogFalloff);
    UE_LOG(LogCarla, Log, TEXT("  - Wetness = %.2f"), Weather.Wetness);
    UE_LOG(LogCarla, Log, TEXT("  - ScatteringIntensity = %.2f"), Weather.ScatteringIntensity);
    UE_LOG(LogCarla, Log, TEXT("  - MieScatteringScale = %.2f"), Weather.MieScatteringScale);
    UE_LOG(LogCarla, Log, TEXT("  - RayleighScatteringScale = %.2f"), Weather.RayleighScatteringScale);
    UE_LOG(LogCarla, Log, TEXT("  - DustStorm = %.2f"), Weather.DustStorm);
#endif // CARLA_WEATHER_EXTRA_LOG

    // Call the blueprint that actually changes the weather.
    RefreshWeather(Weather);

    // record the weather event
    ACarlaRecorder *Recorder = UCarlaStatics::GetRecorder(GetWorld());
    if (Recorder && Recorder->IsEnabled())
    {
        CarlaRecorderWeather RecorderWeather;
        RecorderWeather.Cloudiness              = InWeather.Cloudiness;
        RecorderWeather.Precipitation           = InWeather.Precipitation;
        RecorderWeather.PrecipitationDeposits   = InWeather.PrecipitationDeposits;
        RecorderWeather.WindIntensity           = InWeather.WindIntensity;
        RecorderWeather.SunAzimuthAngle         = InWeather.SunAzimuthAngle;
        RecorderWeather.SunAltitudeAngle        = InWeather.SunAltitudeAngle;
        RecorderWeather.FogDensity              = InWeather.FogDensity;
        RecorderWeather.FogDistance             = InWeather.FogDistance;
        RecorderWeather.FogFalloff              = InWeather.FogFalloff;
        RecorderWeather.Wetness                 = InWeather.Wetness;
        RecorderWeather.ScatteringIntensity     = InWeather.ScatteringIntensity;
        RecorderWeather.MieScatteringScale      = InWeather.MieScatteringScale;
        RecorderWeather.RayleighScatteringScale = InWeather.RayleighScatteringScale;
        RecorderWeather.DustStorm               = InWeather.DustStorm;
        Recorder->AddWeather(RecorderWeather);
    }
}

void AWeather::NotifyWeather(ASensor* Sensor)
{
    CheckWeatherPostProcessEffects();

    // Call the blueprint that actually changes the weather.
    RefreshWeather(Weather);
}

void AWeather::SetWeather(const FWeatherParameters& InWeather)
{
    Weather = InWeather;
}

void AWeather::SetDayNightCycle(const bool& active)
{
    DayNightCycle = active;
}

void AWeather::SetHDRIMode(bool bEnable)
{
    if (bHDRIModeActive == bEnable)
    {
        return;
    }
    bHDRIModeActive = bEnable;

    // Hide the whole BP_Sky actor hierarchy so that, while HDRI mode is active,
    // only the HDRIBackdrop lights the scene. Hiding just this actor is not
    // enough because BP_Sky wraps its directional light (sun) and sky light in
    // child / attached actors, so we hide those too.
    SetSkyHierarchyHidden(bEnable);

    UE_LOG(LogCarla, Log, TEXT("[Weather] HDRI mode %s"),
        bEnable ? TEXT("enabled (sky hidden)") : TEXT("disabled (sky restored)"));
}

void AWeather::SetSkyHierarchyHidden(bool bHidden)
{
    // Gather this actor plus any attached / child actors so we hide the entire
    // BP_Sky hierarchy (sky mesh, sun, sky light, sky atmosphere).
    TArray<AActor*> Actors;
    GetAttachedActors(Actors);
    Actors.AddUnique(this);

    for (int32 Index = 0; Index < Actors.Num(); ++Index)
    {
        AActor* Actor = Actors[Index];
        if (Actor == nullptr)
        {
            continue;
        }

        // Follow child-actor components so wrapped actors are hidden as well.
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (UChildActorComponent* ChildComp = Cast<UChildActorComponent>(Component))
            {
                if (AActor* ChildActor = ChildComp->GetChildActor())
                {
                    Actors.AddUnique(ChildActor);
                }
            }
        }

        Actor->SetActorHiddenInGame(bHidden);
    }
}
