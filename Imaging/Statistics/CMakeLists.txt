set(classes
  vtkImageAccumulate
  vtkImageHistogram
  vtkImageHistogramStatistics)

vtk_module_add_module(VTK::ImagingStatistics
  CLASSES ${classes})
