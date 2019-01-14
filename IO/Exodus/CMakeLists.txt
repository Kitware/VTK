set(classes
  vtkCPExodusIIElementBlock
  vtkCPExodusIIInSituReader
  vtkExodusIICache
  vtkExodusIIReader
  vtkExodusIIReaderParser
  vtkExodusIIReaderVariableCheck
  vtkExodusIIWriter
  vtkModelMetadata)

set(template_classes
  vtkCPExodusIINodalCoordinatesTemplate
  vtkCPExodusIIResultsArrayTemplate)

set(headers
  vtkExodusIIReaderPrivate.h)

vtk_module_add_module(VTK::IOExodus
  CLASSES ${classes}
  TEMPLATE_CLASSES ${template_classes}
  HEADERS ${headers})
