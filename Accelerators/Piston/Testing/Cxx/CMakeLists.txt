
create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestUsePiston
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)
string (REPLACE "vtk" "" _baselinedname ${vtk-module})

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${_baselinedname}/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

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
