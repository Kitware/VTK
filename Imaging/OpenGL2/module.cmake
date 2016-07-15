vtk_module(vtkImagingOpenGL2
  TCL_NAME vtkImagingOpenGLII
  BACKEND
    OpenGL2
  DEPENDS
    vtkImagingGeneral
    vtkRendering${VTK_RENDERING_BACKEND}
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
)
