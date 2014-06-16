if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Rendering)
endif()

vtk_module(vtkGeovisCore
  ${_groups}
  DEPENDS
    vtkIOXML
    vtkInteractionWidgets
    vtkInteractionStyle
    vtkInfovisLayout
    vtkViewsCore
    vtkRenderingOpenGL # For vtkOpenGLHardwareSupport in vtkGeoTerrain
    vtklibproj4
  TEST_DEPENDS
    vtkViewsGeovis
    vtkViewsInfovis
    vtkRenderingCore
    vtkRenderingFreeTypeOpenGL
    vtkTestingRendering
    vtkInteractionStyle
  )
