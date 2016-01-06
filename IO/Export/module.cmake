if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(opengl_depends vtkRenderingGL2PS)
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
    vtkIOExport${VTK_RENDERING_BACKEND}
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingAnnotation
    vtkRenderingLabel
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
  )
