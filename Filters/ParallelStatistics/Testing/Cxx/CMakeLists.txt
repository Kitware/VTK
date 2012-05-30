vtk_module_test_executable(TestRandomPMomentStatisticsMPI TestRandomPMomentStatisticsMPI.cxx)

add_test(NAME ${vtk-module}Cxx-TestRandomPMomentStatisticsMPI
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestRandomPMomentStatisticsMPI>
    ${VTK_MPI_POSTFLAGS})
