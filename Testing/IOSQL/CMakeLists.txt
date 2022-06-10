set(classes
  DatabaseSchemaWith2Tables)

vtk_module_add_module(VTK::TestingIOSQL
  CLASSES ${classes})
vtk_add_test_mangling(VTK::TestingIOSQL)
