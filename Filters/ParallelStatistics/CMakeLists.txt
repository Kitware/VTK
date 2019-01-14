set(classes
  vtkPAutoCorrelativeStatistics
  vtkPContingencyStatistics
  vtkPCorrelativeStatistics
  vtkPDescriptiveStatistics
  vtkPKMeansStatistics
  vtkPMultiCorrelativeStatistics
  vtkPOrderStatistics
  vtkPPCAStatistics
  vtkPBivariateLinearTableThreshold)

vtk_module_add_module(VTK::FiltersParallelStatistics
  CLASSES ${classes})
