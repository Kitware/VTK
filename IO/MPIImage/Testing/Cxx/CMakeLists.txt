if(VTK_DATA_ROOT)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  vtk_module_test_executable(ParallelIsoTest ParallelIso.cxx)
  if(VTK_MPI_MAX_NUMPROCS GREATER 1)
     add_test(NAME ParallelIso-image
       COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
       $<TARGET_FILE:ParallelIsoTest>
       -D ${VTK_DATA_ROOT}
       -T ${VTK_TEST_OUTPUT_DIR}
       -V Baseline/Parallel/ParallelIso.cxx.png
       ${VTK_MPI_POSTFLAGS})
  endif()
endif()
