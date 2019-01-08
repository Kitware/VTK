enable_language(CUDA)

vtk_module_find_package(
  PACKAGE OptiX)

set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --use_fast_math")

set(cuda_sources
  CUDA/Cylinder.cu
  CUDA/PerspectiveCamera.cu
  CUDA/Phong.cu
  CUDA/Sphere.cu
  CUDA/TriangleMesh.cu)
add_library(vtkRenderingOptiX-ptx OBJECT
  ${cuda_sources})
target_link_libraries(vtkRenderingOptiX-ptx
  PRIVATE
    optix)
set_property(TARGET vtkRenderingOptiX-ptx
  PROPERTY
    CUDA_PTX_COMPILATION ON)
_vtk_module_apply_properties(vtkRenderingOptiX-ptx)
_vtk_module_install(vtkRenderingOptiX-ptx
  OBJECTS DESTINATION "lib/ptx")

set(classes
  vtkOptiXActorNode
  vtkOptiXCameraNode
  vtkOptiXCompositePolyDataMapper2Node
  vtkOptiXLightNode
  vtkOptiXPass
  vtkOptiXPolyDataMapperNode
  vtkOptiXPtxLoader
  vtkOptiXRendererNode
  vtkOptiXViewNodeFactory
  vtkOptiXWindowNode)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkOptiXConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOptiXConfig.h"
  @ONLY)

set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkOptiXConfig.h")

vtk_module_add_module(VTK::RenderingOptiX
  CLASSES ${classes}
  HEADERS ${headers})

vtk_module_link(VTK::RenderingOptiX
  PRIVATE
    vtkRenderingOptiX-ptx
    optix)
