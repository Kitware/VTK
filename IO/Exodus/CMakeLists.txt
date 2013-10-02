set(Module_SRCS
  vtkCPExodusIIElementBlock.cxx
  vtkCPExodusIIInSituReader.cxx
  vtkCPExodusIINodalCoordinatesTemplate.h
  vtkCPExodusIIResultsArrayTemplate.h
  vtkExodusIICache.cxx
  vtkExodusIIReader.cxx
  vtkExodusIIReaderParser.cxx
  vtkExodusIIReaderVariableCheck.cxx
  vtkExodusModel.cxx
  vtkExodusIIWriter.cxx
  vtkModelMetadata.cxx
  )

set_source_files_properties(
  vtkCPExodusIINodalCoordinatesTemplate
  vtkCPExodusIIResultsArrayTemplate
  vtkExodusIIReaderParser
  vtkExodusIIReaderVariableCheck
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOExodus ${Module_SRCS})
