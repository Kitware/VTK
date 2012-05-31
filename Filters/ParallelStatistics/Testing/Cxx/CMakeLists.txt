# Build TestRandomPKContingencyStatisticsMPI
vtk_module_test_executable(TestRandomPContingencyStatisticsMPI TestRandomPContingencyStatisticsMPI.cxx)
# Add test
add_test(NAME ${vtk-module}Cxx-TestRandomPContingencyStatisticsMPI
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestRandomPContingencyStatisticsMPI>
    ${VTK_MPI_POSTFLAGS})

# Build TestRandomPKMeansStatisticsMPI
vtk_module_test_executable(TestRandomPKMeansStatisticsMPI TestRandomPKMeansStatisticsMPI.cxx)
# Add test
add_test(NAME ${vtk-module}Cxx-TestRandomPKMeansStatisticsMPI
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestRandomPKMeansStatisticsMPI>
    ${VTK_MPI_POSTFLAGS})

# Build TestRandomPMomentStatisticsMPI
vtk_module_test_executable(TestRandomPMomentStatisticsMPI TestRandomPMomentStatisticsMPI.cxx)
# Add test
add_test(NAME ${vtk-module}Cxx-TestRandomPMomentStatisticsMPI
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestRandomPMomentStatisticsMPI>
    ${VTK_MPI_POSTFLAGS})

# Build TestRandomPOrderStatisticsMPI
vtk_module_test_executable(TestRandomPOrderStatisticsMPI TestRandomPOrderStatisticsMPI.cxx)
# Add test
add_test(NAME ${vtk-module}Cxx-TestRandomPOrderStatisticsMPI
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestRandomPOrderStatisticsMPI>
    ${VTK_MPI_POSTFLAGS})

# Build TestRealDataPDescriptiveStatisticsMPI
vtk_module_test_executable(TestRealDataPDescriptiveStatisticsMPI TestRealDataPDescriptiveStatisticsMPI.cxx)
