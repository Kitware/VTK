set(classes
  vtkTensorPrincipalInvariants
  vtkYieldCriteria)

vtk_module_add_module(VTK::FiltersTensor
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersTensor)
