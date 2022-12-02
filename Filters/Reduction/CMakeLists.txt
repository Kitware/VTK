set(classes
  vtkToImplicitArrayFilter
)

vtk_module_add_module(VTK::FiltersReduction
  CLASSES ${classes}
)
vtk_add_test_mangling(VTK::FiltersReduction)
