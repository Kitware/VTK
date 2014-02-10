if (NOT VTK_LEGACY_REMOVE)
vtk_module(vtkInfovisParallel
  DEPENDS
    vtkInfovisBoost
    vtkInfovisBoostGraphAlgorithms
    vtkCommonExecutionModel
    vtkParallelMPI
    vtkFiltersParallel
    vtkIOSQL
  TEST_DEPENDS
    vtkViewsInfovis
    vtkInfovisLayout
    vtkIOMySQL
    vtkTestingRendering
  EXCLUDE_FROM_ALL
  )
endif()
