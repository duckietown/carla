// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/MsgPack.h"
#include "carla/geom/Vector3D.h"

#include <string>

#ifdef LIBCARLA_INCLUDED_FROM_UE4
#include <compiler/enable-ue4-macros.h>
#include "Carla/HDRI/HDRIParameters.h"
#include <compiler/disable-ue4-macros.h>
#endif // LIBCARLA_INCLUDED_FROM_UE4

namespace carla {
namespace rpc {

  /// HDRI lighting state for a map.
  ///
  /// When @a enabled is true the HDRIBackdrop drives all scene lighting and the
  /// regular sky/weather actor is hidden. @a asset is the name of a cubemap
  /// asset that is resolved against the server's default HDRI directory; an
  /// empty string leaves the current cubemap unchanged.
  class HDRIParameters {
  public:

    HDRIParameters() = default;

    HDRIParameters(
        bool in_enabled,
        std::string in_asset,
        float in_intensity,
        float in_size,
        geom::Vector3D in_projection_center,
        geom::Vector3D in_location)
      : enabled(in_enabled),
        asset(std::move(in_asset)),
        intensity(in_intensity),
        size(in_size),
        projection_center(in_projection_center),
        location(in_location) {}

    bool enabled = false;
    std::string asset;
    float intensity = 1.0f;
    float size = 1000.0f;
    geom::Vector3D projection_center = {0.0f, 0.0f, 0.0f};
    geom::Vector3D location = {0.0f, 0.0f, 0.0f};

#ifdef LIBCARLA_INCLUDED_FROM_UE4

    HDRIParameters(const FHDRIParameters &Params)
      : enabled(Params.bEnabled),
        asset(TCHAR_TO_UTF8(*Params.Asset)),
        intensity(Params.Intensity),
        size(Params.Size),
        projection_center(
            Params.ProjectionCenter.X,
            Params.ProjectionCenter.Y,
            Params.ProjectionCenter.Z),
        location(
            Params.Location.X,
            Params.Location.Y,
            Params.Location.Z) {}

    operator FHDRIParameters() const {
      FHDRIParameters Params;
      Params.bEnabled = enabled;
      Params.Asset = FString(asset.c_str());
      Params.Intensity = intensity;
      Params.Size = size;
      Params.ProjectionCenter = {
          projection_center.x,
          projection_center.y,
          projection_center.z};
      Params.Location = {
          location.x,
          location.y,
          location.z};
      return Params;
    }

#endif // LIBCARLA_INCLUDED_FROM_UE4

    bool operator!=(const HDRIParameters &rhs) const {
      return
          enabled != rhs.enabled ||
          asset != rhs.asset ||
          intensity != rhs.intensity ||
          size != rhs.size ||
          projection_center != rhs.projection_center ||
          location != rhs.location;
    }

    bool operator==(const HDRIParameters &rhs) const {
      return !(*this != rhs);
    }

    MSGPACK_DEFINE_ARRAY(
        enabled,
        asset,
        intensity,
        size,
        projection_center,
        location);
  };

} // namespace rpc
} // namespace carla
