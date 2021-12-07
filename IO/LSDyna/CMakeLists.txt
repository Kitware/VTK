set(classes
  LSDynaFamily
  LSDynaMetaData
  vtkLSDynaReader
  vtkLSDynaSummaryParser)

set(nowrap_classes
  vtkLSDynaPart
  vtkLSDynaPartCollection)

vtk_module_add_module(VTK::IOLSDyna
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes})
