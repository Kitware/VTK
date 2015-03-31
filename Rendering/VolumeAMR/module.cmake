vtk_module(vtkRenderingVolumeAMR
  DEPENDS
    vtkParallelCore
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkFiltersAMR
  KIT
    vtkParallel
  )
