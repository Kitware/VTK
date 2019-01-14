set(classes
  vtkAutoCorrelativeStatistics
  vtkBivariateLinearTableThreshold
  vtkComputeQuartiles
  vtkContingencyStatistics
  vtkCorrelativeStatistics
  vtkDescriptiveStatistics
  vtkHighestDensityRegionsStatistics
  vtkExtractFunctionalBagPlot
  vtkKMeansDistanceFunctorCalculator
  vtkKMeansDistanceFunctor
  vtkKMeansStatistics
  vtkMultiCorrelativeStatistics
  vtkOrderStatistics
  vtkPCAStatistics
  vtkStatisticsAlgorithm
  vtkStrahlerMetric
  vtkStreamingStatistics)

vtk_module_add_module(VTK::FiltersStatistics
  CLASSES ${classes})
