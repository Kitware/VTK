set(Module_SRCS
  vtkContourValues.cxx
  vtkErrorCode.cxx
  vtkFunctionParser.cxx
  vtkHeap.cxx
  vtkPolygonBuilder.cxx
  vtkResourceFileLocator.cxx
  )

vtk_module_library(vtkCommonMisc ${Module_SRCS})
