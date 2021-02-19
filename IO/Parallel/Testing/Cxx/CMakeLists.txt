if (TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkIOParallelCxxTests-MPI tests
    TESTING_DATA
    TestPOpenFOAMReader.cxx
    )
  vtk_test_cxx_executable(vtkIOParallelCxxTests-MPI tests)
endif()

vtk_add_test_cxx(vtkIOParallelCxxTests tests
  TestPOpenFOAMReader.cxx
  TestBigEndianPlot3D.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOParallelCxxTests tests)
