set(classes
  vtkHyperTreeGridMapper)

vtk_module_add_module(VTK::RenderingHyperTreeGrid
  CLASSES ${classes}
  )

if (VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_module_definitions(VTK::RenderingCore
    PRIVATE
      VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
endif()
vtk_add_test_mangling(VTK::RenderingHyperTreeGrid)
