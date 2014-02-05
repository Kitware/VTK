set(Module_SRCS
  vtkCPExodusIIElementBlock.cxx
  vtkCPExodusIIInSituReader.cxx
  vtkCPExodusIINodalCoordinatesTemplate.txx
  vtkCPExodusIIResultsArrayTemplate.txx
  vtkExodusIICache.cxx
  vtkExodusIIReader.cxx
  vtkExodusIIReaderParser.cxx
  vtkExodusIIReaderVariableCheck.cxx
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

set(vtkIOExodus_HDRS
  vtkCPExodusIINodalCoordinatesTemplate.h
  vtkCPExodusIIResultsArrayTemplate.h
)


vtk_module_library(vtkIOExodus ${Module_SRCS})
