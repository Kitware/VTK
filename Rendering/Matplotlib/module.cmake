if(ANDROID OR APPLE_IOS)
  set(gl2ps_depends)
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(gl2ps_depends vtkRenderingGL2PS vtkIOExportOpenGL)
elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(gl2ps_depends vtkRenderingGL2PSOpenGL2 vtkIOExportOpenGL2)
endif()
vtk_module(vtkRenderingMatplotlib
  IMPLEMENTS
    vtkRenderingFreeType
  TEST_DEPENDS
    vtkCommonColor
    vtkInteractionImage
    vtkInteractionWidgets
    vtkIOGeometry
    vtkIOParallel
    vtkTestingRendering
    vtkInteractionStyle
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
    ${gl2ps_depends}
  DEPENDS
    vtkPythonInterpreter
    vtkRenderingFreeType
  PRIVATE_DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonTransforms
    vtkImagingCore
    vtkPython
    vtkRenderingCore
    vtkWrappingPythonCore
  )