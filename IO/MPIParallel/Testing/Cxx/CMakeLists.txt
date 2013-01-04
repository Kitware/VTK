if (VTK_LARGE_DATA_ROOT)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  vtk_module_test_executable(vtkIOMPIParallelCxx-TestPWindBladeReader TestPWindBladeReader.cxx)
  add_test(NAME TestPWindBladeReader
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:TestPWindBladeReader>
    -D ${VTK_LARGE_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/TestWindBladeReader.png
    ${VTK_MPI_POSTFLAGS})
endif()
