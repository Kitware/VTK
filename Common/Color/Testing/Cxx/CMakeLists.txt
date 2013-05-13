vtk_add_test_cxx(NO_VALID
  TestCategoricalColors.cxx
  TestNamedColors.cxx
  )

if (VTK_DATA_ROOT)
  vtk_add_test_cxx(Common
    TestColorSeries.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
