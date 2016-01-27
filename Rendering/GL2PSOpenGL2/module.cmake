vtk_module(vtkRenderingGL2PSOpenGL2
  TCL_NAME vtkRenderingGLtoPSOpenGLII
  IMPLEMENTS
    vtkRenderingOpenGL2
  BACKEND
    OpenGL2
  DEPENDS
    vtkCommonCore
    vtkImagingCore
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkRenderingFreeType
    vtkgl2ps
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
)
