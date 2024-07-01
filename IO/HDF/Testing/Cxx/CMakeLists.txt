vtk_add_test_cxx(vtkIOHDFCxxTests tests
  TestHDFReader.cxx,NO_VALID,NO_OUTPUT
  TestHDFReaderTemporal.cxx,NO_VALID,NO_OUTPUT
  TestHDFWriter.cxx,NO_VALID
  TestHDFWriterTemporal.cxx,NO_VALID
  )

if (TARGET VTK::ParallelMPI)
  set(vtkIOHDFCxxTests-MPI_NUMPROCS 3)
  vtk_add_test_mpi(vtkIOHDFCxxTests-MPI mpi_test
    TestHDFWriterDistributed.cxx,TESTING_DATA,NO_VALID
  )

  vtk_test_cxx_executable(vtkIOHDFCxxTests-MPI mpi_test)
endif()

vtk_test_cxx_executable(vtkIOHDFCxxTests tests)
