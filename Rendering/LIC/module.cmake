if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Rendering)
endif()

vtk_module(vtkRenderingLIC
  ${_groups}
  DEPENDS
    vtkIOXML
    vtkIOLegacy
    vtkImagingSources
    vtkRenderingOpenGL
  PRIVATE_DEPENDS
    vtksys
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkOpenGL
  )
