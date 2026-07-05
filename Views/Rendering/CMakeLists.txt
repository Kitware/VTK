set(classes
  vtkSurfaceRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsRendering
  CLASSES ${classes})
