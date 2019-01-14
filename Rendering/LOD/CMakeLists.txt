set(classes
  vtkLODActor
  vtkQuadricLODActor)

vtk_module_add_module(VTK::RenderingLOD
  CLASSES ${classes})
