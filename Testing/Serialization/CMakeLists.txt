set(classes
  vtkSerDesMock
  vtkSerDesMockObject)

vtk_module_add_module(VTK::TestingSerialization
  CLASSES ${classes})
vtk_add_test_mangling(VTK::TestingSerialization)
