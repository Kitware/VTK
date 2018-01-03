set(classes
  vtkSMPContourGrid
  vtkSMPMergePoints
  vtkSMPMergePolyDataHelper)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    vtkSMPContourGridManyPieces
    vtkThreadedSynchronizedTemplates3D
    vtkThreadedSynchronizedTemplatesCutter3D
    vtkSMPTransform
    vtkSMPWarpVector)
endif()

vtk_module_add_module(VTK::FiltersSMP
  CLASSES ${classes})
