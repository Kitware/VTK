if(TARGET pvtkpython)
  add_test(NAME ${vtk-module}Cxx-testParallelXMLWriters
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
      $<TARGET_FILE:pvtkpython> ${CMAKE_CURRENT_SOURCE_DIR}/testParallelXMLWriters.py
      ${VTK_MPI_POSTFLAGS})
endif()
