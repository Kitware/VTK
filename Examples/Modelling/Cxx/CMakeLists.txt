cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(Modelling)

find_package(VTK
  COMPONENTS
    CommonCore
    FiltersGeneral
    IOXML
    ImagingHybrid
    RenderingCore
  OPTIONAL_COMPONENTS
    TestingRendering)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(finance MACOSX_BUNDLE finance.cxx)
target_link_libraries(finance PRIVATE ${vtk_modules})

add_executable(Delaunay3D MACOSX_BUNDLE Delaunay3D.cxx)
target_link_libraries(Delaunay3D PRIVATE ${vtk_modules})

add_executable(Delaunay3DAlpha MACOSX_BUNDLE Delaunay3DAlpha.cxx)
target_link_libraries(Delaunay3DAlpha PRIVATE ${vtk_modules})

vtk_module_autoinit(
  TARGETS finance
          Delaunay3D
          Delaunay3DAlpha
  MODULES ${vtk_modules})

if (TARGET VTK::TestingRendering)
  add_subdirectory(Testing)
endif ()
