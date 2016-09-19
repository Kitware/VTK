
vtk_module(vtkIOSQL
  GROUPS
    StandAlone
  TEST_DEPENDS
    vtkIOLegacy
    vtkTestingIOSQL
    vtkTestingCore
  KIT
    vtkIO
  DEPENDS
    vtkCommonCore
    vtkCommonExecutionModel
    vtkIOCore
  PRIVATE_DEPENDS
    vtksqlite
    vtksys
  )
