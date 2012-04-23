set(Module_SRCS
  vtkConvertSelectionDomain.cxx
  vtkDataRepresentation.cxx
  vtkEmptyRepresentation.cxx
  vtkRenderViewBase.cxx
  vtkView.cxx
  vtkViewTheme.cxx
  )

vtk_module_library(vtkViewsCore ${Module_SRCS})
