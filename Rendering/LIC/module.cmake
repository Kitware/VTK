if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Rendering)
endif()

vtk_module(vtkRenderingLIC
  ${_groups}
  BACKEND
    OpenGL
  DEPENDS
    vtkIOXML
    vtkIOLegacy
    vtkImagingSources
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtksys
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
  )
