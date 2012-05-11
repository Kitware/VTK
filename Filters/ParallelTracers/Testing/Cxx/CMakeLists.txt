vtk_module_test_executable(TestPStream TestPStream.cxx)

add_test(NAME ${vtk-module}Cxx-TestPStream-image
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestPStream>
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/TestPStream.cxx.png
    ${VTK_MPI_POSTFLAGS})

vtk_module_test_executable(TestPStreamGeometry TestVectorFieldSource.cxx
  TestPStreamGeometry.cxx)
add_test(NAME ${vtk-module}Cxx-TestPStreamGeometry
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestPStreamGeometry>
    ${VTK_MPI_POSTFLAGS})

vtk_module_test_executable(TestPParticleTracers TestPParticleTracers.cxx)
add_test(NAME ${vtk-module}Cxx-TestPParticleTracers
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestPParticleTracers>
    ${VTK_MPI_POSTFLAGS})

vtk_module_test_executable(TestPStreamAMR TestPStreamAMR.cxx)
add_test(NAME ${vtk-module}Cxx-TestPStreamAMR
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestPStreamAMR>
    -D ${VTK_DATA_ROOT}
    ${VTK_MPI_POSTFLAGS})
