vtk_module(vtkGeovisCore
  GROUPS
    Rendering
  DEPENDS
    vtkInteractionWidgets
    vtkInteractionStyle
    vtkInfovisLayout
    vtkViewsCore
    vtkRenderingOpenGL # For vtkOpenGLHardwareSupport in vtkGeoTerrain
  )
