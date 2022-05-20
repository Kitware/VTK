set(classes
  vtkPAutoCorrelativeStatistics
  vtkPBivariateLinearTableThreshold
  vtkPComputeQuantiles
  vtkPComputeQuartiles
  vtkPContingencyStatistics
  vtkPCorrelativeStatistics
  vtkPDescriptiveStatistics
  vtkPKMeansStatistics
  vtkPMultiCorrelativeStatistics
  vtkPOrderStatistics
  vtkPPCAStatistics)

vtk_module_add_module(VTK::FiltersParallelStatistics
  CLASSES ${classes})
