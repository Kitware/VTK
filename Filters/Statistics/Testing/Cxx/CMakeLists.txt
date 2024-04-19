vtk_add_test_cxx(vtkFiltersStatisticsCxxTests tests
  TestLengthDistribution.cxx,NO_VALID,NO_OUTPUT
)

vtk_add_test_cxx(vtkFiltersStatisticsCxxTests no_data_tests
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
set(all_tests ${tests} ${no_data_tests})
vtk_test_cxx_executable(vtkFiltersStatisticsCxxTests all_tests)
