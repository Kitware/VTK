# Indicate that we want to compile the R version of the test
add_definitions(-DUSE_GNU_R)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  # These tests are copies from Filters/Statistics but perform R testing when USE_GNU_R is defined
  TestCorrelativeStatistics.cxx
  TestContingencyStatistics.cxx
  TestPCAStatistics.cxx
  TestDescriptiveStatistics.cxx

  TestRCalculatorFilter.cxx
  TestRCalculatorFilterAsymmetrical.cxx
  TestRInterface.cxx
  TestRRandomTableSource.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
