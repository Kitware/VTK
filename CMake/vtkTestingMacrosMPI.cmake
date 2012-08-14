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
