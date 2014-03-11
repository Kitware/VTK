cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()

project (VolumeRendering)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkIOImage
  vtkIOXML
  vtkInteractionWidgets
  vtkRenderingVolumeOpenGL
)
include(${VTK_USE_FILE})

set(RENDERING_EXAMPLES_SRCS
  GPURenderDemo
  FixedPointVolumeRayCastMapperCT
)

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES})
endforeach()



