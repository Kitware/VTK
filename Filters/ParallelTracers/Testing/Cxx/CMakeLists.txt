vtk_module_test_executable(TestPStream TestPStream.cxx)

add_test(TestPStream-image
  ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
  TestPStream
  -D ${VTK_DATA_ROOT}
  -T ${VTK_TEST_OUTPUT_DIR}
  -V Baseline/Parallel/TestPStream.cxx.png
  ${VTK_MPI_POSTFLAGS})


vtk_module_test_executable(TestPStreamGeometry TestVectorFieldSource.cxx TestPStreamGeometry.cxx)

add_test(TestPStreamGeometry
  ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
  TestPStreamGeometry
  ${VTK_MPI_POSTFLAGS})

vtk_module_test_executable(TestPStreamAMR TestPStreamAMR.cxx)

add_test(TestPStreamAMR
  ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
  TestPStreamGeometry
  ${VTK_MPI_POSTFLAGS})