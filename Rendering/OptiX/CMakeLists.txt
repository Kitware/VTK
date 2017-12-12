cmake_minimum_required(VERSION 3.8)

find_package( OptiX REQUIRED )
find_package( CUDA 4.0 REQUIRED )

include_directories(
  "${OptiX_INCLUDE}"
  "${CUDA_INCLUDE_DIRS}"
  )


set( PTX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib/ptx" CACHE PATH
  "Fallback path to where OptiX stores generated PTX from CUDA files"
  )
mark_as_advanced(PTX_OUTPUT_DIR)

set( CUDA_GENERATED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib/ptx" )

# Set up build .cu -> .ptx build
set( CUDA_NVCC_ARCH "sm_30" CACHE STRING "The architecture for which OptiX source is compiled." )
set( CUDA_NVCC_FLAGS "-arch ${CUDA_NVCC_ARCH} --use_fast_math" )

set(OPTIX_SRCS
  CUDA/PerspectiveCamera.cu
  CUDA/Phong.cu
  CUDA/TriangleMesh.cu
  CUDA/Sphere.cu
  CUDA/Cylinder.cu
  )

CUDA_COMPILE_PTX( ptx_files ${OPTIX_SRCS} )

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/vtkOptiXConfig.h.in"
  "${CMAKE_BINARY_DIR}/include/vtkOptiXConfig.h"
  )

include_directories(
    "${CMAKE_BINARY_DIR}/include"
    )

set(Module_SRCS
  vtkOptiXActorNode.cxx
  vtkOptiXCameraNode.cxx
  vtkOptiXCompositePolyDataMapper2Node.cxx
  vtkOptiXLightNode.cxx
  vtkOptiXPass.cxx
  vtkOptiXPolyDataMapperNode.cxx
  vtkOptiXRendererNode.cxx
  vtkOptiXViewNodeFactory.cxx
  vtkOptiXWindowNode.cxx
  vtkOptiXPtxLoader.cxx
  )

vtk_module_library(vtkRenderingOptiX
  ${Module_SRCS}
  ${ptx_files}
  )

target_link_libraries(${vtk-module} LINK_PUBLIC ${optix_LIBRARY})

install(FILES ${ptx_files}
  DESTINATION lib/ptx )
