set(classes
  vtkAutoCorrelativeStatistics
  vtkBivariateLinearTableThreshold
  vtkComputeQuartiles
  vtkContingencyStatistics
  vtkCorrelativeStatistics
  vtkDescriptiveStatistics
  vtkExtractFunctionalBagPlot
  vtkHighestDensityRegionsStatistics
  vtkKMeansDistanceFunctor
  vtkKMeansDistanceFunctorCalculator
  vtkKMeansStatistics
  vtkMultiCorrelativeStatistics
  vtkOrderStatistics
  vtkPCAStatistics
  vtkStatisticsAlgorithm
  vtkStrahlerMetric
  vtkStreamingStatistics)

set(headers
  vtkStatisticsAlgorithmPrivate.h)

vtk_module_add_module(VTK::FiltersStatistics
  CLASSES ${classes}
  HEADERS ${headers})
