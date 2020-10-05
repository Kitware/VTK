if(TARGET VTK::ParallelMPI)
vtk_add_test_python_mpi(
  TestPIOReader.py
  )
endif()
