# -----------------------------------------------------------------------------
# Macro vtk_tests() takes a list of cxx files which will be driven by the modules
# test driver. This helps reduce a lot of boiler place code in each module
macro(vtk_tests)
  create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${ARGV}
    EXTRA_INCLUDE vtkTestDriver.h)

  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    if(VTK_DATA_ROOT)
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${vtk-module}/${TName}.png)
    else()
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        ${${TName}_ARGS})
    endif()
  endforeach()
endmacro(vtk_tests)

# -----------------------------------------------------------------------------
# add_test_mpi macro take one or more files. It uses the files name for the name
# of the test. The test will be run using MPI.
macro (add_test_mpi fileName)
  get_filename_component(name ${fileName} NAME_WE)
  vtk_module_test_executable(
    ${name}
    ${name}.cxx
    ${ARGN})
  add_test(
    NAME ${vtk-module}Cxx-MPI-${name}
    COMMAND ${VTK_MPIRUN_EXE}
    ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    $<TARGET_FILE:${name}>
    ${VTK_MPI_POSTFLAGS})
endmacro()
