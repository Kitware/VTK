set(classes
  vtkWebGPUVolumeRayCastMapper)

set(private_headers
  "Private/vtkWebGPUVolumeRayCastMapperInternals.h")

set(private_sources
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
