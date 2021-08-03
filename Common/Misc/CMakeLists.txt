set(classes
  vtkContourValues
  vtkErrorCode
  vtkExprTkFunctionParser
  vtkFunctionParser
  vtkHeap
  vtkPolygonBuilder
  vtkResourceFileLocator)

vtk_module_add_module(VTK::CommonMisc
  CLASSES ${classes})
