set(classes
  vtkHyperTreeGridMapper)

vtk_module_add_module(VTK::RenderingHyperTree
  CLASSES ${classes}
  )

if (VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_module_definitions(VTK::RenderingCore
    PRIVATE
      VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
endif()
