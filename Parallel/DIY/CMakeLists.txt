set(classes
  vtkDIYDataExchanger
  vtkDIYExplicitAssigner
  vtkDIYGhostUtilities
  vtkDIYUtilities)

set(templates
  vtkDIYGhostUtilities.txx
  vtkDIYUtilities.txx)

vtk_module_add_module(VTK::ParallelDIY
  CLASSES ${classes}
  TEMPLATES ${templates})

vtk_module_definitions(VTK::ParallelDIY
  PUBLIC DIY_NO_THREADS)
