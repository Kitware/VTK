if(ANDROID OR APPLE_IOS) # No GL2PS on mobile
  return()
endif()
vtk_module(vtkRenderingGL2PS
  TCL_NAME vtkRenderingGLtoPS
  BACKEND
    OpenGL
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
  DEPENDS
    vtkCommonCore
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkCommonDataModel
    vtkCommonMath
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingContext2D
    vtkRenderingCore
    vtkRenderingFreeType
    vtkgl2ps
)