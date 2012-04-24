vtk_module_test_executable(TestPStream TestPStream.cxx)

add_test(TestPStream-image
  ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
  TestPStream
  -D ${VTK_DATA_ROOT}
  -T ${VTK_TEST_OUTPUT_DIR}
  -V Baseline/Parallel/TestPStream.cxx.png
  ${VTK_MPI_POSTFLAGS})
