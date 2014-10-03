if (NOT ANDROID AND NOT APPLE_IOS)
  set(include_in_backend BACKEND OpenGL2)
endif()

vtk_module(vtkRenderingFreeTypeOpenGL2
  IMPLEMENTS
    vtkRenderingCore
  TCL_NAME
    vtkRenderingFreeTypeOpenGLII
  ${include_in_backend}
  DEPENDS
    vtkRenderingFreeType
    vtkRenderingOpenGL2
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
  )
