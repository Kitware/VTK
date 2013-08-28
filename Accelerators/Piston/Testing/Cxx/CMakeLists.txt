vtk_add_test_cxx(
  TestUsePiston.cxx
)

vtk_test_cxx_executable(${vtk-module}CxxTests)

if(VTK_MPI_MAX_NUMPROCS GREATER 1)
  include(vtkMPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})

  set(PistonMPITests TestDMPFiltering TestCompositeRender)

  foreach(test ${PistonMPITests})

    vtk_module_test_executable(${vtk-module}Cxx-${test} ${test}.cxx)
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Cxx-${test}
      COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
      $<TARGET_FILE:${vtk-module}Cxx-${test}>
      -D ${VTK_TEST_DATA_DIR}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V DATA{../Data/Baseline/${test}.png,:}
      ${VTK_MPI_POSTFLAGS})
    vtk_mpi_link(${vtk-module}Cxx-${test})
  endforeach()

endif()
