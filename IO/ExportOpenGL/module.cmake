if(ANDROID OR APPLE_IOS) # No GL2PS on mobile
  return()
endif()

vtk_module(vtkIOExportOpenGL
  IMPLEMENTS
    vtkIOExport
  BACKEND
    OpenGL
  DEPENDS
    vtkCommonCore
    vtkIOExport
    vtkRenderingAnnotation
    vtkRenderingContext2D
    vtkRenderingCore
    vtkRenderingFreeType
    vtkRenderingGL2PS
    vtkRenderingLabel
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkgl2ps
  TEST_DEPENDS
    vtkTestingRendering
    vtkViewsContext2D
  EXCLUDE_FROM_WRAPPING
)
