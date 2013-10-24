vtk_module(vtkRenderingGL2PS
  TCL_NAME vtkRenderingGLtoPS
  DEPENDS
    vtkRenderingContext2D
  PRIVATE_DEPENDS
    vtkRenderingOpenGL
    vtkRenderingFreeType
    vtkgl2ps
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
)
