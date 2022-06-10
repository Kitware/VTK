set(classes
  vtkContextInteractorStyle
  vtkContextView
  )

vtk_module_add_module(VTK::ViewsContext2D
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ViewsContext2D)
