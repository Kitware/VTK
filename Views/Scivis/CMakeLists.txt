set(classes
  vtkStandardRenderView
  vtkSurfaceRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsScivis
  CLASSES ${classes})
