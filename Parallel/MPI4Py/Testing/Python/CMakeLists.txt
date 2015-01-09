add_test(NAME ${vtk-module}Python-TestParallelNumpy
  COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:pvtkpython> ${CMAKE_CURRENT_SOURCE_DIR}/TestParallelNumpy.py
    ${VTK_MPI_POSTFLAGS})
