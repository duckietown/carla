// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include <carla/rpc/HDRIParameters.h>

#include <ostream>

namespace carla {
namespace rpc {

  std::ostream &operator<<(std::ostream &out, const HDRIParameters &hdri) {
    out << "HDRIParameters(enabled=" << (hdri.enabled ? "True" : "False")
        << ", asset=" << hdri.asset
        << ", intensity=" << std::to_string(hdri.intensity)
        << ", size=" << std::to_string(hdri.size)
        << ", projection_center=(" << std::to_string(hdri.projection_center.x)
        << ", " << std::to_string(hdri.projection_center.y)
        << ", " << std::to_string(hdri.projection_center.z) << ")"
        << ", location=(" << std::to_string(hdri.location.x)
        << ", " << std::to_string(hdri.location.y)
        << ", " << std::to_string(hdri.location.z) << "))";
    return out;
  }

} // namespace rpc
} // namespace carla
