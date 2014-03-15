# Indicate that we want to compile the R version of the test
add_definitions(-DUSE_GNU_R)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  # These tests are in Filters/Statistics but perform R testnig when USE_GNU_R is defined
  ${vtkFiltersStatistics_SOURCE_DIR}/Testing/Cxx/TestCorrelativeStatistics.cxx
  ${vtkFiltersStatistics_SOURCE_DIR}/Testing/Cxx/TestContingencyStatistics.cxx
  ${vtkFiltersStatistics_SOURCE_DIR}/Testing/Cxx/TestPCAStatistics.cxx
  ${vtkFiltersStatistics_SOURCE_DIR}/Testing/Cxx/TestDescriptiveStatistics.cxx

  TestRCalculatorFilter.cxx
  TestRInterface.cxx
  TestRRandomTableSource.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
