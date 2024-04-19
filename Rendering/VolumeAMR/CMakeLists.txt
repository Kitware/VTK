set(classes
  vtkAMRVolumeMapper)

vtk_module_add_module(VTK::RenderingVolumeAMR
  CLASSES ${classes})
vtk_add_test_mangling(VTK::RenderingVolumeAMR)
