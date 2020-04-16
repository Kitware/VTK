set(classes
  vtkSMPContourGrid
  vtkSMPMergePoints
  vtkSMPMergePolyDataHelper)

vtk_module_add_module(VTK::FiltersSMP
  CLASSES ${classes})
