set(classes
  vtkFidesReader)

vtk_module_add_module(VTK::IOFides
  CLASSES           ${classes})

vtk_module_link(VTK::IOFides PRIVATE VTK::fides)
