vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
  TestAutoCorrelativeStatistics.cxx
  TestCorrelativeStatistics.cxx
  TestContingencyStatistics.cxx
  TestDescriptiveStatistics.cxx
  TestHighestDensityRegionsStatistics.cxx
  TestExtractFunctionalBagPlot.cxx
  TestKMeansStatistics.cxx
  TestMultiCorrelativeStatistics.cxx
  TestOrderStatistics.cxx
  TestPCAStatistics.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
