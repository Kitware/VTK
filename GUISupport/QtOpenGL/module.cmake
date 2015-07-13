if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Qt)
endif()
vtk_module(vtkGUISupportQtOpenGL
  ${_groups}
  DEPENDS
    vtkGUISupportQt
    vtkRendering${VTK_RENDERING_BACKEND}
  TEST_DEPENDS
    vtkTestingCore
  EXCLUDE_FROM_WRAPPING
  )
