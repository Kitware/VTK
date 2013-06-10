vtk_module(vtkRenderingGL2PS
  TCL_NAME vtkRenderingGLtoPS
  DEPENDS
    vtkRenderingContext2D
    vtkRenderingOpenGL
    vtkgl2ps
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
)
