set(Module_SRCS
  vtkImageCanvasSource2D.cxx
  vtkImageEllipsoidSource.cxx
  vtkImageGaussianSource.cxx
  vtkImageGridSource.cxx
  vtkImageMandelbrotSource.cxx
  vtkImageNoiseSource.cxx
  vtkImageSinusoidSource.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
