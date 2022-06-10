set(classes
  vtkImageButterworthHighPass
  vtkImageButterworthLowPass
  vtkImageFFT
  vtkImageFourierCenter
  vtkImageFourierFilter
  vtkImageIdealHighPass
  vtkImageIdealLowPass
  vtkImageRFFT)

vtk_module_add_module(VTK::ImagingFourier
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingFourier)
