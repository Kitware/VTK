if(ANDROID OR APPLE_IOS) # No GL2PS on mobile
  return()
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(gl2ps_depends vtkRenderingGL2PS)
  set(gl2ps_test_depends vtkIOExportOpenGL)
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(gl2ps_depends vtkRenderingGL2PSOpenGL2)
  set(gl2ps_test_depends vtkIOExportOpenGL2)
endif()
vtk_module(vtkRenderingGL2PS
  TCL_NAME vtkRenderingGLtoPS
  BACKEND
    OpenGL
  TEST_DEPENDS
    vtkTestingRendering
    vtkInteractionStyle
    vtkIOExport
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingGL2PS
    vtkIOExportOpenGL
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
