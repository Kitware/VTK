if(TARGET VTK::ParallelMPI)
  set(testParallelXMLWriters_NUMPROCS 2)
  vtk_add_test_python_mpi(
      testParallelXMLWriters.py,NO_VALID
      testParallelMultiBlockWriter.py,NO_VALID
  )
endif()
vtk_add_test_python(
  TestPXMLTimeInField.py,NO_DATA,NO_VALID,NO_RT
)
