vtk_module(vtkIOSQL
  GROUPS
    StandAlone
  DEPENDS
    vtkIOCore
  PRIVATE_DEPENDS
    vtksys
    vtksqlite # We should consider splitting this into a module.
  TEST_DEPENDS
    vtkIOLegacy
    vtkTestingIOSQL
    vtkTestingCore
  KIT
    vtkIO
  )
