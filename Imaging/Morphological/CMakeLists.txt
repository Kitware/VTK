set(Module_SRCS
  vtkImageConnector.cxx
  vtkImageContinuousDilate3D.cxx
  vtkImageContinuousErode3D.cxx
  vtkImageDilateErode3D.cxx
  vtkImageIslandRemoval2D.cxx
  vtkImageNonMaximumSuppression.cxx
  vtkImageOpenClose3D.cxx
  vtkImageSeedConnectivity.cxx
  vtkImageSkeleton2D.cxx
  vtkImageThresholdConnectivity.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
