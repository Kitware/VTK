include(vtkMPI)

set(TestPStream_NUMPROCS 2)
set(TestPStreamGeometry_NUMPROCS 4)
set(TestPParticleTracers_NUMPROCS 2)
set(TestPStreamAMR_NUMPROCS 4)
vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TestPStream.cxx,TESTING_DATA
  TestPStreamGeometry.cxx
  TestPParticleTracers.cxx
  TestPStreamAMR
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests
  TestVectorFieldSource.cxx)
