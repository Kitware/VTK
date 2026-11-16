set(classes
  vtkWebGPUVolumeRayCastCoordinator
  vtkWebGPUVolumeRayCastMapper)

set(private_headers
  "Private/vtkWebGPUVolumeBrickPool.h"
  "Private/vtkWebGPUVolumeRayCastContribution.h"
  "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.h"
  "Private/vtkWebGPUVolumeRayCastMapperInput.h"
  "Private/vtkWebGPUVolumeRayCastMapperInternals.h")

set(private_sources
  "Private/vtkWebGPUVolumeBrickPool.cxx"
  "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.cxx"
  "Private/vtkWebGPUVolumeRayCastMapperInput.cxx"
  "Private/vtkWebGPUVolumeRayCastMapperInternals.cxx")

set(shader_files
  wgsl/VolumeRayCast.wgsl)

set(sources)
foreach (shader_file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${shader_file}"
    EXPORT_HEADER "vtkRenderingVolumeWebGPUModule.h"
    EXPORT_SYMBOL "VTKRENDERINGVOLUMEWEBGPU_NO_EXPORT"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()

vtk_object_factory_declare(
  BASE "vtkGPUVolumeRayCastMapper"
  OVERRIDE "vtkWebGPUVolumeRayCastMapper")

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGVOLUMEWEBGPU_EXPORT")

vtk_module_add_module(VTK::RenderingVolumeWebGPU
  CLASSES ${classes}
  SOURCES ${sources} ${private_sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header})

vtk_module_compile_features(VTK::RenderingVolumeWebGPU PUBLIC cxx_std_17)

# This module calls the wgpu* entry points directly, and VTK links no
# implementation, so those names are not bound at link time. VTK::RenderingWebGPU
# redirects them to the table it resolves at runtime with a forced include; do
# the same here so the calls reach that table rather than an undefined symbol.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_webgpu_dispatch_header
    "${CMAKE_CURRENT_SOURCE_DIR}/../WebGPU/Private/vtkWebGPUProcDispatch.h")
  if(MSVC)
    set(_vtk_webgpu_force_include "/FI${_vtk_webgpu_dispatch_header}")
  else()
    set(_vtk_webgpu_force_include "-include${_vtk_webgpu_dispatch_header}")
  endif()
  vtk_module_compile_options(VTK::RenderingVolumeWebGPU
    PRIVATE "${_vtk_webgpu_force_include}")
endif()
