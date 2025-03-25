set(classes
  vtkGridAxesActor3D
  vtkGridAxesActor2D
  vtkGridAxesHelper
  vtkGridAxesPlaneActor2D)

vtk_module_add_module(VTK::RenderingGridAxes
  CLASSES ${classes})
vtk_add_test_mangling(VTK::RenderingGridAxes)
