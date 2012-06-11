set(MyTests
    TestPStructuredGridConnectivity.cxx
    TestPStructuredGridGhostDataGenerator.cxx
    TestPUniformGridGhostDataGenerator.cxx
    )

foreach(test ${MyTests})
  get_filename_component(TName ${test} NAME_WE)
  vtk_module_test_executable(${TName} ${test})
  add_test(
    NAME ${vtk-module}Cxx-${TName}
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
    ${MPIEXEC_PREFLAGS} $<TARGET_FILE:${TName}> ${VTK_MPI_POSTFLAGS}
    )
endforeach()
