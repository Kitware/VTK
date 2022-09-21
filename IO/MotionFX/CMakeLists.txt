set(classes
  vtkMotionFXCFGReader)

vtk_module_add_module(VTK::IOMotionFX
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IOMotionFX)
