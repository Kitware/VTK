cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT (Rendering)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build Rendering examples without VTK_SOURCE_DIR")
endif()
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
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
