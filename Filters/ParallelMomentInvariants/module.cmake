if (NOT WIN32) # disabled since vtkFiltersParallelDIY2 doesn't support WIN32
vtk_module(vtkFiltersParallelMomentInvariants
  TEST_DEPENDS
    vtkIOXML
  DEPENDS
    vtkFiltersCore
    vtkImagingCore
    vtkFiltersMomentInvariants
    vtkParallelCore
    vtkFiltersParallelDIY2
    vtkParallelMPI
  PRIVATE_DEPENDS
    vtkeigen
  )
endif()
