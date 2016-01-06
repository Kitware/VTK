if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(opengl_depends vtkRenderingGL2PS)
  set(private_opengl_depends vtkgl2ps)
  set(test_opengl_depends vtkIOExportOpenGL)
endif()
vtk_module(vtkIOExport
  GROUPS
    Rendering
  DEPENDS
    vtkCommonCore
    ${opengl_depends}
    vtkImagingCore
    vtkRenderingCore
  PRIVATE_DEPENDS
    vtkIOImage
    vtkFiltersGeometry
    ${private_opengl_depends}
  TEST_DEPENDS
    vtkCommonColor
    vtkChartsCore
    vtkInteractionImage
    ${test_opengl_depends}
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingAnnotation
    vtkRenderingLabel
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
  )
