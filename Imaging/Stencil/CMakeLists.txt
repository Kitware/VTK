set(classes
  vtkImageStencil
  vtkImageStencilToImage
  vtkImageToImageStencil
  vtkImplicitFunctionToImageStencil
  vtkLassoStencilSource
  vtkPolyDataToImageStencil
  vtkROIStencilSource)

vtk_module_add_module(VTK::ImagingStencil
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingStencil)
