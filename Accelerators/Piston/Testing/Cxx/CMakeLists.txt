vtk_add_test_cxx(AcceleratorsPiston
  TestUsePiston.cxx
)

vtk_test_cxx_executable(${vtk-module}CxxTests)

if(VTK_DATA_ROOT AND (VTK_MPI_MAX_NUMPROCS GREATER 1))
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})

  set(PistonMPITests TestDMPFiltering TestCompositeRender)

  foreach(test ${PistonMPITests})

    vtk_module_test_executable(${vtk-module}Cxx-${test} ${test}.cxx)
    if(VTK_DATA_ROOT)
      add_test(NAME ${vtk-module}Cxx-${test}
        COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
        $<TARGET_FILE:${vtk-module}Cxx-${test}>
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${_baselinedname}/${test}.png
        ${VTK_MPI_POSTFLAGS})
    else()
      add_test(NAME ${vtk-module}Cxx-${test}
        COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
        $<TARGET_FILE:${vtk-module}Cxx-${test}>
        ${VTK_MPI_POSTFLAGS})
    endif()
  endforeach()

endif()
