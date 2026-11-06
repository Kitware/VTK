vtk_add_test_cxx(vtkIOHDFCxxTests tests
  TestHDFReader.cxx,NO_VALID
  TestHDFReaderTemporal.cxx,NO_VALID,NO_OUTPUT
  TestHDFWriter.cxx,NO_VALID
  TestHDFWriterTemporal.cxx,NO_VALID
  TestHDFWriterHTGTemporal.cxx,NO_VALID
  TestHDFWriterTableTemporal.cxx,NO_VALID
  TestHDFWriterChangingTopology.cxx,NO_VALID
  TestHDFWriterPolyhedra.cxx,NO_VALID
  )

if (TARGET VTK::ParallelMPI)
  set(configuration_tests
    TestHDFWriterUGConfigs.cxx,TESTING_DATA,NO_VALID
    TestHDFWriterPDConfigs.cxx,TESTING_DATA,NO_VALID
    TestHDFWriterPolyhedraConfigs.cxx,TESTING_DATA,NO_VALID
    TestHDFWriterCompositeConfigs.cxx,TESTING_DATA,NO_VALID
  )
  set(shared_test_sources
    HDFWriterDistributedTestConfigurations.cxx
    HDFWriterDistributedTestSource.cxx
    HDFWriterDistributedTestUtilities.cxx
  )

  vtk_add_test_cxx(vtkIOHDFCxxTests tests ${configuration_tests})

  set(vtkIOHDFCxxTests-MPI_NUMPROCS 3)
  vtk_add_test_mpi(vtkIOHDFCxxTests-MPI mpi_test
    TestHDFConfigurationSource.cxx,NO_VALID,NO_OUTPUT
    TestHDFWriterDistributed.cxx,TESTING_DATA,NO_VALID
    TestHDFWriterHTGDistributed.cxx,TESTING_DATA,NO_VALID
    TestHDFReaderDistributed.cxx,TESTING_DATA,NO_VALID
    ${configuration_tests}
  )

  vtk_test_cxx_executable(vtkIOHDFCxxTests-MPI mpi_test
    ${shared_test_sources}
  )

endif()

vtk_test_cxx_executable(vtkIOHDFCxxTests tests ${shared_test_sources})
