cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT (Modelling)
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkCommonDataModel
  vtkFiltersGeneral
  vtkIOXML
  vtkImagingHybrid
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingRendering
)
include(${VTK_USE_FILE})

add_executable(finance MACOSX_BUNDLE finance.cxx)
target_link_libraries(finance ${VTK_LIBRARIES})

add_executable(Delaunay3D MACOSX_BUNDLE Delaunay3D.cxx)
target_link_libraries(Delaunay3D ${VTK_LIBRARIES})

add_executable(Delaunay3DAlpha MACOSX_BUNDLE Delaunay3DAlpha.cxx)
target_link_libraries(Delaunay3DAlpha ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(vtk-example ModellingExamples)
    set(TestFinance_ARGS "DATA{${VTK_TEST_INPUT_DIR}/financial.txt}")
    set(TestDelaunay3D_ARGS
      "DATA{${VTK_TEST_INPUT_DIR}/cow.vtp}"
      ${VTK_TEST_OUTPUT_DIR}/Delaunay3D.vtu
      )
    set(TestDelaunay3DAlpha_ARGS
      .5 "DATA{${VTK_TEST_INPUT_DIR}/cow.vtp}"
      ${VTK_TEST_OUTPUT_DIR}/Delaunay3DAlpha.vtu
      )
    vtk_add_test_cxx(${vtk-example}CxxTests data_tests
      TestFinance.cxx
      )
    vtk_add_test_cxx(${vtk-example}CxxTests tests
      NO_DATA NO_VALID NO_OUTPUT
      TestDelaunay3D.cxx
      TestDelaunay3DAlpha.cxx
      )
    set(all_tests
      ${data_tests}
      ${tests}
      )
    vtk_test_cxx_executable(${vtk-example}CxxTests all_tests
      RENDERING_FACTORY
      )
  endif()
endif()
