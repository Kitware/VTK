set(classes
  vtkImageAccumulate
  vtkImageHistogram
  vtkImageHistogramStatistics)

vtk_module_add_module(VTK::ImagingStatistics
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingStatistics)
