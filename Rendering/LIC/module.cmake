if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Rendering)
endif()
vtk_module(vtkRenderingLIC
  LEGACY 8.1 "Please switch to using `OpenGL2` VTK_RENDERING_BACKEND to replace this module."
  ${_groups}
  BACKEND
    OpenGL
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkCommonMath
    vtkCommonSystem
    vtkIOCore
    vtkIOLegacy
    vtkIOXML
    vtkImagingCore
    vtkImagingSources
    vtkRenderingCore
    vtksys
  )
