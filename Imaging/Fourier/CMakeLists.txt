set(classes
  vtkImageButterworthHighPass
  vtkImageButterworthLowPass
  vtkImageFFT
  vtkImageFourierCenter
  vtkImageFourierFilter
  vtkImageIdealHighPass
  vtkImageIdealLowPass
  vtkImageRFFT
  vtkTableFFT)

vtk_module_add_module(VTK::ImagingFourier
  CLASSES ${classes})
