set(classes
  vtkImageDivergence
  vtkImageDotProduct
  vtkImageLogarithmicScale
  vtkImageLogic
  vtkImageMagnitude
  vtkImageMaskBits
  vtkImageMathematics
  vtkImageWeightedSum)

vtk_module_add_module(VTK::ImagingMath
  CLASSES ${classes})
