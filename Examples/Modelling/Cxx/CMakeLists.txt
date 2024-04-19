cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Modelling)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

find_package(VTK
  COMPONENTS
    CommonCore
    FiltersGeneral
    IOXML
    ImagingHybrid
    InteractionStyle
    RenderingCore
    RenderingOpenGL2
    vtksys
  OPTIONAL_COMPONENTS
    TestingRendering)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(finance MACOSX_BUNDLE finance.cxx)
target_link_libraries(finance PRIVATE ${VTK_LIBRARIES})

add_executable(Delaunay3D MACOSX_BUNDLE Delaunay3D.cxx)
target_link_libraries(Delaunay3D PRIVATE ${VTK_LIBRARIES})

add_executable(Delaunay3DAlpha MACOSX_BUNDLE Delaunay3DAlpha.cxx)
target_link_libraries(Delaunay3DAlpha PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS finance
          Delaunay3D
          Delaunay3DAlpha
  MODULES ${VTK_LIBRARIES})

if (BUILD_TESTING)
  include(CTest)
  if (TARGET VTK::TestingRendering)

    ######## Regression Testing ########
    set(ExternalData_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/ExternalData)

    list(APPEND ExternalData_URL_TEMPLATES
      # Data published by Girder
      "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"

      # Data published by developers using git-gitlab-push.
      "https://www.vtk.org/files/ExternalData/%(algo)/%(hash)"
    )
    include(ExternalData)

    ExternalData_add_test(ModellingData
      NAME    "ModellingExampleCxx-finance"
      COMMAND finance
        DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/financial.txt}
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/Baseline/finance.png}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_property(TEST ModellingExampleCxx-finance
      PROPERTY
        ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")

    ExternalData_Add_Target(ModellingData)
  endif ()
endif ()
