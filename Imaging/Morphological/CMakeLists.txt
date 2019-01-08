set(classes
  vtkImageConnector
  vtkImageConnectivityFilter
  vtkImageContinuousDilate3D
  vtkImageContinuousErode3D
  vtkImageDilateErode3D
  vtkImageIslandRemoval2D
  vtkImageNonMaximumSuppression
  vtkImageOpenClose3D
  vtkImageSeedConnectivity
  vtkImageSkeleton2D
  vtkImageThresholdConnectivity)

vtk_module_add_module(VTK::ImagingMorphological
  CLASSES ${classes})
