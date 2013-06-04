vtk_add_test_cxx(NO_VALID
  TestCategoricalColors.cxx
  TestNamedColors.cxx
  )

vtk_add_test_cxx(
  TestColorSeries.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
