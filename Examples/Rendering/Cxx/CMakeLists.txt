cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

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
  MaterialObjects
  )

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${VTK_LIBRARIES} )
endforeach()

if(VTK_SOURCE_DIR)
  include_directories(
    # in support of SurfaceLICDemo that includes
    # TestSurfaceLIC.h from the dir below
    ${VTK_SOURCE_DIR}/Rendering/HybridOpenGL/Testing/Cxx
  )
  add_executable(SurfaceLICDemo MACOSX_BUNDLE   SurfaceLICDemo.cxx)
  target_link_libraries(SurfaceLICDemo ${VTK_LIBRARIES} )
endif()
