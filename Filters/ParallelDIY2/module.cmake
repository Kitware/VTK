if (NOT WIN32)
  vtk_module(vtkFiltersParallelDIY2
    TCL_NAME vtkFiltersParallelDIYII
    IMPLEMENTS
      vtkFiltersCore
    GROUPS
      MPI
    TEST_DEPENDS
      vtkFiltersParallelMPI
      vtkInteractionStyle
      vtkRendering${VTK_RENDERING_BACKEND}
      vtkRenderingParallel
      vtkTestingCore
      vtkTestingRendering
    KIT
      vtkParallel
    DEPENDS
      vtkFiltersCore
      vtkdiy2
    PRIVATE_DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelMPI
    )
endif()