vtk_module(vtkViewsContext2D
  TCL_NAME vtkViewsContextIID
  GROUPS
    Views
    Rendering
  KIT
    vtkViews
  DEPENDS
    vtkCommonCore
    vtkRenderingCore
    vtkViewsCore
  PRIVATE_DEPENDS
    vtkRenderingContext2D
  )