set(classes
  vtkCPExodusIIElementBlock
  vtkCPExodusIIInSituReader
  vtkExodusIICache
  vtkExodusIIReader
  vtkExodusIIReaderParser
  vtkExodusIIWriter
  vtkModelMetadata)

set(template_classes
  vtkCPExodusIINodalCoordinatesTemplate
  vtkCPExodusIIResultsArrayTemplate)

set(nowrap_classes
  vtkExodusIIReaderVariableCheck)

set(nowrap_headers
  vtkExodusIIReaderPrivate.h)

vtk_module_add_module(VTK::IOExodus
  CLASSES ${classes}
  TEMPLATE_CLASSES ${template_classes}
  NOWRAP_CLASSES ${nowrap_classes}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::IOExodus)
