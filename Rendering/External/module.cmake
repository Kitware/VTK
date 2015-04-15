vtk_module(vtkRenderingExternal
  DEPENDS
    vtkRenderingCore
    vtkRendering${VTK_RENDERING_BACKEND}
  TEST_DEPENDS
    vtkTestingRendering
  EXCLUDE_FROM_ALL
  )
