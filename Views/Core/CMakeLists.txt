set(classes
  vtkConvertSelectionDomain
  vtkDataRepresentation
  vtkEmptyRepresentation
  vtkRenderViewBase
  vtkView
  vtkViewTheme
  )

vtk_module_add_module(VTK::ViewsCore
  CLASSES ${classes})
