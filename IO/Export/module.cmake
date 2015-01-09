if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(opengl_depends vtkRenderingGL2PS)
  set(private_opengl_depends vtkgl2ps)
endif()
vtk_module(vtkIOExport
  GROUPS
    Rendering
  DEPENDS
    vtkCommonCore
    vtkRenderingAnnotation
    vtkRenderingContext2D
    vtkRenderingCore
    vtkRenderingFreeType
    ${opengl_depends}
    vtkRenderingLabel
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkImagingCore
  PRIVATE_DEPENDS
    vtkIOImage
    vtkFiltersGeometry
    ${private_opengl_depends}
  TEST_DEPENDS
    vtkCommonColor
    vtkChartsCore
    vtkInteractionImage
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingAnnotation
    vtkRenderingFreeType${VTK_RENDERING_BACKEND}
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
  )
