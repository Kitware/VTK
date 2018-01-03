set(classes
  vtkImageAnisotropicDiffusion2D
  vtkImageAnisotropicDiffusion3D
  vtkImageCheckerboard
  vtkImageCityBlockDistance
  vtkImageConvolve
  vtkImageCorrelation
  vtkImageEuclideanDistance
  vtkImageEuclideanToPolar
  vtkImageGaussianSmooth
  vtkImageGradient
  vtkImageGradientMagnitude
  vtkImageHybridMedian2D
  vtkImageLaplacian
  vtkImageMedian3D
  vtkImageNormalize
  vtkImageRange3D
  vtkImageSeparableConvolution
  vtkImageSobel2D
  vtkImageSobel3D
  vtkImageSpatialAlgorithm
  vtkImageVariance3D
  vtkSimpleImageFilterExample
  vtkImageSlab
  vtkImageSlabReslice)

vtk_module_add_module(VTK::ImagingGeneral
  CLASSES ${classes})
