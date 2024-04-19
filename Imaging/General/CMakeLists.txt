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
  vtkImageSlab
  vtkImageSlabReslice
  vtkImageSobel2D
  vtkImageSobel3D
  vtkImageSpatialAlgorithm
  vtkImageVariance3D)

vtk_module_add_module(VTK::ImagingGeneral
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ImagingGeneral)
