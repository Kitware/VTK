##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

find_package(anari 0.15.0 REQUIRED)

## Build device target ##

set (sources
  ViskoresDevice.cpp
  ViskoresDeviceGlobalState.cpp
  ViskoresLibrary.cpp
  Object.cpp
  array/Array1D.cpp
  array/Array2D.cpp
  array/Array3D.cpp
  array/ObjectArray.cpp
  camera/Camera.cpp
  camera/Orthographic.cpp
  camera/Perspective.cpp
  renderer/Renderer.cpp
  scene/Group.cpp
  scene/Instance.cpp
  scene/World.cpp
  scene/light/Light.cpp
  scene/surface/Surface.cpp
  scene/surface/geometry/Geometry.cpp
  scene/surface/material/Material.cpp
  scene/surface/material/MatteMaterial.cpp
  scene/surface/material/sampler/Image1DSampler.cpp
  scene/surface/material/sampler/Sampler.cpp
  scene/volume/Volume.cpp
  scene/volume/spatial_field/SpatialField.cpp
  scene/volume/spatial_field/StructuredRegularField.cpp
  )

set(device_sources
  array/ArrayConversion.cpp
  frame/Frame.cpp
  scene/surface/geometry/Cylinder.cpp
  scene/surface/geometry/Quad.cpp
  scene/surface/geometry/Sphere.cpp
  scene/surface/geometry/Triangle.cpp
  scene/volume/TransferFunction1D.cpp
  )

viskores_library(
  NAME anari_library_viskores
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  )

target_link_libraries(anari_library_viskores
PUBLIC
  anari::helium
  )

include(GenerateExportHeader)
generate_export_header(anari_library_viskores
  EXPORT_MACRO_NAME "VISKORES_LIBRARY_INTERFACE"
)

target_include_directories(anari_library_viskores
PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

## Code generation ##

anari_generate_queries(
  DEVICE_TARGET anari_library_viskores
  CPP_NAMESPACE viskores_device
  JSON_DEFINITIONS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/viskores_device.json
)

## Installation ##

install(TARGETS anari_library_viskores
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
