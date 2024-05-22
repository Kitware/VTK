vtk_module_find_package(
  PACKAGE anari
  VERSION 0.9.1
  FORWARD_VERSION_REQ MINOR
  CONFIG_MODE)

option(VTK_ANARI_ENABLE_NVTX "Enable NVTX instrumentation" OFF)
mark_as_advanced(VTK_ANARI_ENABLE_NVTX)

if(VTK_ANARI_ENABLE_NVTX)
  # If not found, set CUDAToolkit_ROOT env variable.
  # e.g. export CUDAToolkit_ROOT=<path>
  vtk_module_find_package(
    PACKAGE CUDAToolkit
    COMPONENTS CUDA::nvToolsExt)
endif()

set(classes
  vtkAnariActorNode
  vtkAnariCameraNode
  vtkAnariCompositePolyDataMapperNode
  vtkAnariLightNode
  vtkAnariPass
  vtkAnariPolyDataMapperNode
  vtkAnariRendererNode
  vtkAnariViewNodeFactory
  vtkAnariWindowNode
  vtkAnariFollowerNode
  vtkAnariProfiling
  vtkAnariVolumeNode
  vtkAnariVolumeMapperNode
  vtkAnariVolumeMapper
  )

set(nowrap_classes
  vtkAnariProfiling
)

vtk_object_factory_declare(
  BASE vtkAnariVolumeInterface
  OVERRIDE vtkAnariVolumeMapper
)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGANARI_EXPORT"
)

vtk_module_add_module(VTK::RenderingAnari
  HEADER_DIRECTORIES
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  NOWRAP_CLASSES ${nowrap_classes}
  PRIVATE_HEADERS ${vtk_object_factory_header})

vtk_module_link(VTK::RenderingAnari
  PUBLIC
    anari::anari
    VTK::FiltersGeometry
  PRIVATE
    $<$<BOOL:${VTK_ANARI_ENABLE_NVTX}>:CUDA::nvToolsExt>
)

vtk_module_definitions(VTK::RenderingAnari
  PRIVATE
    $<$<BOOL:${VTK_ANARI_ENABLE_NVTX}>:USE_NVTX>
)

vtk_add_test_mangling(VTK::RenderingAnari)
