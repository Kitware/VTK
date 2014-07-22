vtk_module(vtkIOXdmf3
  TCL_NAME vtkIOXdmfIII
  GROUPS
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkIOLegacy
  PRIVATE_DEPENDS
    vtksys
    vtkxdmf3
  TEST_DEPENDS
    vtkFiltersGeneral
    vtkTestingCore
    vtkTestingRendering
    vtkParallelMPI
  )
