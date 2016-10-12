vtk_module(vtkImagingOpenGL2
  TCL_NAME vtkImagingOpenGLII
  BACKEND
    OpenGL2
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
  DEPENDS
    vtkImagingGeneral
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkRenderingOpenGL2
)