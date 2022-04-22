set(classes
  vtkAutoCorrelativeStatistics
  vtkBivariateLinearTableThreshold
  vtkComputeQuantiles
  vtkComputeQuartiles
  vtkContingencyStatistics
  vtkCorrelativeStatistics
  vtkDescriptiveStatistics
  vtkExtractFunctionalBagPlot
  vtkExtractHistogram
  vtkHighestDensityRegionsStatistics
  vtkKMeansDistanceFunctor
  vtkKMeansDistanceFunctorCalculator
  vtkKMeansStatistics
  vtkLengthDistribution
  vtkMultiCorrelativeStatistics
  vtkOrderStatistics
  vtkPCAStatistics
  vtkStatisticsAlgorithm
  vtkStrahlerMetric
  vtkStreamingStatistics)

set(nowrap_headers
  vtkStatisticsAlgorithmPrivate.h)

vtk_module_add_module(VTK::FiltersStatistics
  CLASSES ${classes}
  NOWRAP_HEADERS ${nowrap_headers})
