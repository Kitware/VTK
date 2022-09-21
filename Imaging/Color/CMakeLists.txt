set(classes
  vtkImageHSIToRGB
  vtkImageHSVToRGB
  vtkImageLuminance
  vtkImageMapToRGBA
  vtkImageMapToWindowLevelColors
  vtkImageQuantizeRGBToIndex
  vtkImageRGBToHSI
  vtkImageRGBToHSV
  vtkImageRGBToYIQ
  vtkImageYIQToRGB)

vtk_module_add_module(VTK::ImagingColor
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingColor)
