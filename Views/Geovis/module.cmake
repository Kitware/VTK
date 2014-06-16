if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_groups GROUPS Rendering Views)
endif()
vtk_module(vtkViewsGeovis
  ${_groups}
  DEPENDS
    vtkViewsInfovis
    vtkGeovisCore
  )
