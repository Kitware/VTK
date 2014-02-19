set(Module_SRCS
  vtkAutoCorrelativeStatistics.cxx
  vtkBivariateLinearTableThreshold.cxx
  vtkComputeQuartiles.cxx
  vtkContingencyStatistics.cxx
  vtkCorrelativeStatistics.cxx
  vtkDescriptiveStatistics.cxx
  vtkHighestDensityRegionsStatistics.cxx
  vtkExtractFunctionalBagPlot.cxx
  vtkKMeansDistanceFunctorCalculator.cxx
  vtkKMeansDistanceFunctor.cxx
  vtkKMeansStatistics.cxx
  vtkMultiCorrelativeStatistics.cxx
  vtkOrderStatistics.cxx
  vtkPCAStatistics.cxx
  vtkStatisticsAlgorithm.cxx
  vtkStrahlerMetric.cxx
  vtkStreamingStatistics.cxx
  )

set_source_files_properties(
  vtkStatisticsAlgorithm.cxx
  ABSTRACT
)

vtk_module_library(vtkFiltersStatistics ${Module_SRCS})
