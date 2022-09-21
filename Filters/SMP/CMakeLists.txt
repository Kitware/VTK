set(classes
  vtkSMPContourGrid
  vtkSMPMergePoints
  vtkSMPMergePolyDataHelper)

vtk_module_add_module(VTK::FiltersSMP
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersSMP)
