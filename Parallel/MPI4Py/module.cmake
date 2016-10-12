if (VTK_WRAP_PYTHON)
  vtk_module(vtkParallelMPI4Py
    GROUPS
      MPI
    COMPILE_DEPENDS
      vtkmpi4py
    EXCLUDE_FROM_TCL_WRAPPING
    EXCLUDE_FROM_JAVA_WRAPPING
    KIT
      vtkWrapping
    DEPENDS
      vtkCommonCore
      vtkPython
    PRIVATE_DEPENDS
      vtkParallelMPI
    )
endif ()