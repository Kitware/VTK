if(ANDROID OR APPLE_IOS) # No GL2PS on mobile
  return()
endif()
vtk_module(vtkRenderingGL2PSOpenGL2
  TCL_NAME vtkRenderingGLtoPSOpenGLII
  IMPLEMENTS
    vtkRenderingOpenGL2
  BACKEND
    OpenGL2
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
  DEPENDS
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonMath
    vtkRenderingCore
    vtkRenderingOpenGL2
    vtkgl2ps
)