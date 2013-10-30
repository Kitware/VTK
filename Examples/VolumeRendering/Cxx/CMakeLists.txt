cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # remove if CMake >= 2.8.11 required
endif()

PROJECT (VolumeRendering)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkIOImage
    vtkIOXML
    vtkImagingCore
    vtkInteractionWidgets
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkRenderingVolume
  )
  include(${VTK_USE_FILE})
endif()

set(RENDERING_EXAMPLES_SRCS
  GPURenderDemo
  FixedPointVolumeRayCastMapperCT
)

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES})
endforeach()



