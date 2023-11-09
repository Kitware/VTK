if(TARGET VTK::ParallelMPI)
  vtk_module_test_data(
    Data/uGridEx.vtk)

  add_subdirectory(Cxx)
endif()
