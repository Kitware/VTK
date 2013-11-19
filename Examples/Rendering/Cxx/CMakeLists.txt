cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Rendering)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build Rendering examples without VTK_SOURCE_DIR")
endif()

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

set(RENDERING_EXAMPLES_SRCS
  Cylinder
  SpecularSpheres
  DiffuseSpheres
  AmbientSpheres
#  MaterialObjects
  )

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES} )
endforeach()
