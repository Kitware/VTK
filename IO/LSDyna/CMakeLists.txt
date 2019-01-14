set(classes
  LSDynaFamily
  LSDynaMetaData
  vtkLSDynaPart
  vtkLSDynaPartCollection
  vtkLSDynaReader
  vtkLSDynaSummaryParser)

vtk_module_add_module(VTK::IOLSDyna
  CLASSES ${classes}
  SOURCES ${sources})
