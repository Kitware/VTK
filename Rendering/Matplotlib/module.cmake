if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(gl2ps_depends vtkRenderingGL2PS)
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(gl2ps_depends vtkRenderingGL2PSOpenGL2)
endif()

vtk_module(vtkRenderingMatplotlib
  IMPLEMENTS
    vtkRenderingFreeType
  DEPENDS
    vtkImagingCore
    vtkRenderingCore
    vtkPythonInterpreter
  PRIVATE_DEPENDS
    vtkWrappingPythonCore
  TEST_DEPENDS
    vtkCommonColor
    vtkInteractionImage
    vtkInteractionWidgets
    vtkIOExport${VTK_RENDERING_BACKEND}
    vtkIOGeometry
    vtkIOParallel
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
    ${gl2ps_depends}
  )
