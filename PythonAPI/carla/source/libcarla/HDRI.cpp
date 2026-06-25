// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include <carla/rpc/HDRIParameters.h>
#include <carla/geom/Vector3D.h>

#include <ostream>

namespace carla {
namespace rpc {

  std::ostream &operator<<(std::ostream &out, const HDRIParameters &hdri);

} // namespace rpc
} // namespace carla

void export_hdri() {
  using namespace boost::python;
  namespace cr = carla::rpc;
  namespace cg = carla::geom;

  class_<cr::HDRIParameters>("HDRIParameters")
    .def(init<bool, std::string, float, float, cg::Vector3D, cg::Vector3D>(
        (arg("enabled")=false,
         arg("asset")=std::string(),
         arg("intensity")=1.0f,
         arg("size")=1000.0f,
         arg("projection_center")=cg::Vector3D(),
         arg("location")=cg::Vector3D())))
    .def_readwrite("enabled", &cr::HDRIParameters::enabled)
    .def_readwrite("asset", &cr::HDRIParameters::asset)
    .def_readwrite("intensity", &cr::HDRIParameters::intensity)
    .def_readwrite("size", &cr::HDRIParameters::size)
    .def_readwrite("projection_center", &cr::HDRIParameters::projection_center)
    .def_readwrite("location", &cr::HDRIParameters::location)
    .def("__eq__", &cr::HDRIParameters::operator==)
    .def("__ne__", &cr::HDRIParameters::operator!=)
    .def(self_ns::str(self_ns::self))
  ;
}
