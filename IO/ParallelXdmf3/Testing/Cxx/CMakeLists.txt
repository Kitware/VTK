if(VTK_MPI_MAX_NUMPROCS GREATER 1 AND VTK_USE_LARGE_DATA)

  include(vtkMPI)

  set(Xdmf3MPITests TestXdmf3Parallel)

  foreach(test ${Xdmf3MPITests})

    vtk_module_test_executable(vtkIOParallelXdmf3Cxx-${test} ${test}.cxx)
    ExternalData_Expand_Arguments(VTKData _
      "DATA{${VTK_TEST_INPUT_DIR}/XDMF/Iron/,REGEX:.*}"
      )
    ExternalData_add_test(VTKData
      NAME vtkIOParallelXdmf3Cxx-${test}
      COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
      $<TARGET_FILE:vtkIOParallelXdmf3Cxx-${test}>
      -D ${VTK_TEST_DATA_DIR}
      -T ${VTK_TEST_OUTPUT_DIR}
#      -V DATA{../Data/Baseline/${test}.png,:}
      ${VTK_MPI_POSTFLAGS})
    vtk_mpi_link(vtkIOParallelXdmf3Cxx-${test})
  endforeach()

endif()
