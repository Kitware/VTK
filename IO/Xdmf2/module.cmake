vtk_module(vtkIOXdmf2
  TCL_NAME vtkIOXdmfII
  GROUPS
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkFiltersExtraction
    vtkIOLegacy
    vtkIOXML
  PRIVATE_DEPENDS
    vtksys
    vtkxdmf2
  TEST_DEPENDS
    vtkFiltersGeneral
  )
