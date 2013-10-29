cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # remove if CMake >= 2.8.11 required
endif()

PROJECT (Rendering)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build Rendering examples without VTK_SOURCE_DIR")
endif()

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkFiltersSources
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkInteractionStyle
  )
  include(${VTK_USE_FILE})
endif()

set(RENDERING_EXAMPLES_SRCS
  Cylinder
  SpecularSpheres
  DiffuseSpheres
  AmbientSpheres
  #MaterialObjects
  )

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES} )
endforeach()
