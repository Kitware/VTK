if (NOT WIN32)
  vtk_module(vtkdiy2
    GROUPS
      MPI
    EXCLUDE_FROM_WRAPPING
  )
endif()