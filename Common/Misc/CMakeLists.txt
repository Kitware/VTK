set(Module_SRCS
  vtkContourValues.cxx
  vtkErrorCode.cxx
  vtkFunctionParser.cxx
  vtkHeap.cxx
  vtkPolygonBuilder.cxx
  )

set_source_files_properties(
  vtkErrorCode.cxx
  vtkPolygonBuilder.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkCommonMisc ${Module_SRCS})
