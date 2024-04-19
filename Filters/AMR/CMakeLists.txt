set(classes
  vtkAMRCutPlane
  vtkAMRGaussianPulseSource
  vtkAMRResampleFilter
  vtkAMRSliceFilter
  vtkAMRToMultiBlockFilter
  vtkImageToAMR
  vtkParallelAMRUtilities)

vtk_module_add_module(VTK::FiltersAMR
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersAMR)
