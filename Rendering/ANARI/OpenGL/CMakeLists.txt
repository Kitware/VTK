set(classes
  vtkAnariPass
)

vtk_module_add_module(VTK::RenderingAnariOpenGL
  CLASSES ${classes}
)

vtk_module_link(VTK::RenderingAnariOpenGL
  PUBLIC
    VTK::RenderingAnariCore
)
