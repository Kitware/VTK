set(TestPLagrangianParticleTracker_NUMPROCS 2)
set(TestPStream_NUMPROCS 2)
set(TestPStreamGeometry_NUMPROCS 4)
set(TestPParticleTracers_NUMPROCS 2)
set(TestPStreamAMR_NUMPROCS 4)
vtk_add_test_mpi(vtkFiltersParallelFlowPathsCxxTests-MPI tests
  TestPLagrangianParticleTracker.cxx,TESTING_DATA
  TestPStream.cxx,TESTING_DATA
  TestPStreamGeometry.cxx
  TestPParticleTracers.cxx
  TestPStreamAMR.cxx,TESTING_DATA,NO_VALID
  )
vtk_test_cxx_executable(vtkFiltersParallelFlowPathsCxxTests-MPI tests
  TestVectorFieldSource.cxx)
