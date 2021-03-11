set(classes
  vtkPAutoCorrelativeStatistics
  vtkPBivariateLinearTableThreshold
  vtkPContingencyStatistics
  vtkPCorrelativeStatistics
  vtkPDescriptiveStatistics
  vtkPKMeansStatistics
  vtkPMultiCorrelativeStatistics
  vtkPOrderStatistics
  vtkPPCAStatistics)

vtk_module_add_module(VTK::FiltersParallelStatistics
  CLASSES ${classes})
