cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Modelling)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkFiltersCore
    vtkFiltersGeneral
    vtkIOXML
    vtkImagingHybrid
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkInteractionStyle
)
  include(${VTK_USE_FILE})
endif()

add_executable(finance MACOSX_BUNDLE finance.cxx)
target_link_libraries(finance ${VTK_LIBRARIES})

add_executable(Delaunay3D MACOSX_BUNDLE Delaunay3D.cxx)
target_link_libraries(Delaunay3D ${VTK_LIBRARIES})

add_executable(Delaunay3DAlpha MACOSX_BUNDLE Delaunay3DAlpha.cxx)
target_link_libraries(Delaunay3DAlpha ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(KIT ModellingExamples)
    set(MyTests TestFinance.cxx TestDelaunay3D.cxx TestDelaunay3DAlpha.cxx)

    include(vtkTestingObjectFactory)

    add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
    target_link_LIBRARIES(${KIT}CxxTests ${VTK_LIBRARIES})

    if (VTK_DATA_ROOT)
      add_test(NAME ${KIT}-Finance COMMAND ${KIT}CxxTests
               TestFinance ${VTK_DATA_ROOT}/Data/financial.txt)
      add_test(NAME ${KIT}-Delaunay3D COMMAND ${KIT}CxxTests
               TestDelaunay3D
               ${VTK_DATA_ROOT}/Data/cow.vtp
               ${VTK_TEST_OUTPUT_DIR}/Delaunay3D.vtu)
      add_test(NAME ${KIT}-Delaunay3DAlpa COMMAND ${KIT}CxxTests
               TestDelaunay3DAlpha
               .5 ${VTK_DATA_ROOT}/Data/cow.vtp
               ${VTK_TEST_OUTPUT_DIR}/Delaunay3DAlpha.vtu)
    endif()
  endif()
endif()
