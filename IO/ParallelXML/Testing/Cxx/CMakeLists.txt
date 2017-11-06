# note, to enable testing the vtkParallelMPI module should be enabled

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestParallelUnstructuredGridIO.cxx,No_DATA,NO_VALID
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
