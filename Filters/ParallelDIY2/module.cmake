if (NOT WIN32)
  vtk_module(vtkFiltersParallelDIY2
    TCL_NAME vtkFiltersParallelDIYII
    IMPLEMENTS
      vtkFiltersCore
    GROUPS
      MPI
    DEPENDS
      vtkFiltersCore
    PRIVATE_DEPENDS
      vtkdiy2
    TEST_DEPENDS
      vtkFiltersParallelMPI
      vtkInteractionStyle
      vtkParallelMPI
      vtkRendering${VTK_RENDERING_BACKEND}
      vtkRenderingParallel
      vtkTestingCore
      vtkTestingRendering
    KIT
      vtkParallel
    )
endif()
