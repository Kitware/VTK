vtk_add_test_cxx(vtkFiltersStatisticsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestAutoCorrelativeStatistics.cxx
  TestComputeQuantiles.cxx
  TestComputeQuartiles.cxx
  TestContingencyStatistics.cxx
  TestCorrelativeStatistics.cxx
  TestDescriptiveStatistics.cxx
  TestExtractFunctionalBagPlot.cxx
  TestExtractHistogram.cxx
  TestHighestDensityRegionsStatistics.cxx
  TestKMeansStatistics.cxx
  TestMultiCorrelativeStatistics.cxx
  TestOrderStatistics.cxx
  TestPCAStatistics.cxx
  )
vtk_test_cxx_executable(vtkFiltersStatisticsCxxTests tests)
