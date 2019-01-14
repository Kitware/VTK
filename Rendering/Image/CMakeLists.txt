set(classes
  vtkDepthImageToPointCloud
  vtkImageResliceMapper
  vtkImageSliceCollection
  vtkImageStack)

vtk_module_add_module(VTK::RenderingImage
  CLASSES ${classes})
