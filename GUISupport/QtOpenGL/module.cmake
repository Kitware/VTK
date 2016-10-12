if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Qt)
endif()
vtk_module(vtkGUISupportQtOpenGL
  ${_groups}
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkFiltersSources
  EXCLUDE_FROM_WRAPPING
  DEPENDS
    vtkCommonCore
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkGUISupportQt
    vtkInteractionStyle
  )