set(Module_SRCS
  vtkExodusIICache.cxx
  vtkExodusIIReader.cxx
  vtkExodusIIReaderParser.cxx
  vtkExodusIIReaderVariableCheck.cxx
  vtkExodusModel.cxx
  vtkExodusIIWriter.cxx
  vtkModelMetadata.cxx
  )

set_source_files_properties(
  vtkExodusIIReaderParser
  vtkExodusIIReaderVariableCheck
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOExodus ${Module_SRCS})
