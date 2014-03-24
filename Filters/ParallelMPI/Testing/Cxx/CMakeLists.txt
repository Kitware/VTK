include(vtkMPI)

vtk_add_test_mpi(TestImplicitConnectivity.cxx)
vtk_mpi_link(TestImplicitConnectivity)
