vtk_add_test_cxx(vtkIOEnSightCxxTests tests
  TestEnSightReaderStaticMeshCache.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkIOEnSightCxxTests tests
  RENDERING_FACTORY
  )

if (TARGET VTK::ParallelMPI)
  set(TestEnSightSOSReaderMandelbrotMPI_NUMPROCS 2)
  set(TestEnSightCombinedReaderMPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkIOEnSightCxxTests-MPI mpiTests TESTING_DATA
    TestEnSightSOSReaderMandelbrotMPI.cxx
    TestEnSightCombinedReaderMPI.cxx
  )
  vtk_test_cxx_executable(vtkIOEnSightCxxTests-MPI mpiTests)
endif()
