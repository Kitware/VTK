set(Module_SRCS
  vtkImageAnisotropicDiffusion2D.cxx
  vtkImageAnisotropicDiffusion3D.cxx
  vtkImageCheckerboard.cxx
  vtkImageCityBlockDistance.cxx
  vtkImageConvolve.cxx
  vtkImageCorrelation.cxx
  vtkImageEuclideanDistance.cxx
  vtkImageEuclideanToPolar.cxx
  vtkImageGaussianSmooth.cxx
  vtkImageGradient.cxx
  vtkImageGradientMagnitude.cxx
  vtkImageHybridMedian2D.cxx
  vtkImageLaplacian.cxx
  vtkImageMedian3D.cxx
  vtkImageNormalize.cxx
  vtkImageRange3D.cxx
  vtkImageSeparableConvolution.cxx
  vtkImageSobel2D.cxx
  vtkImageSobel3D.cxx
  vtkImageSpatialAlgorithm.cxx
  vtkImageVariance3D.cxx
  vtkSimpleImageFilterExample.cxx
  vtkImageSlab.cxx
  vtkImageSlabReslice.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
