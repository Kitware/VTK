if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(opengl_depends vtkRenderingGL2PS)
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(opengl_depends vtkRenderingGL2PSOpenGL2)
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
  TEST_DEPENDS
    vtkCommonColor
    vtkChartsCore
    vtkInteractionImage
    vtkIOExport${VTK_RENDERING_BACKEND}
    vtkIOParallel
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingAnnotation
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkRenderingLabel
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
  )
