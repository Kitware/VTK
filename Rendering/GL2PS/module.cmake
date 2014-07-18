vtk_module(vtkRenderingGL2PS
  TCL_NAME vtkRenderingGLtoPS
  DEPENDS
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingFreeType
    vtkgl2ps
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
)
