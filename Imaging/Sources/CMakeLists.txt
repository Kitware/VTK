set(classes
  vtkImageCanvasSource2D
  vtkImageEllipsoidSource
  vtkImageGaussianSource
  vtkImageGridSource
  vtkImageMandelbrotSource
  vtkImageNoiseSource
  vtkImageSinusoidSource)

vtk_module_add_module(VTK::ImagingSources
  CLASSES ${classes})
