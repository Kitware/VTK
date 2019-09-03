set(classes
  vtkDIYUtilities
  vtkDIYExplicitAssigner)

vtk_module_add_module(VTK::ParallelDIY
  CLASSES ${classes})

vtk_module_definitions(VTK::ParallelDIY
  PUBLIC DIY_NO_THREADS)
