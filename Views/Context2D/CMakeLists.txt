set(classes
  vtkContextView
  vtkContextInteractorStyle
  )

vtk_module_add_module(VTK::ViewsContext2D
  CLASSES ${classes})
