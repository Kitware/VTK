vtk_add_test_cxx(vtkIOCGNSReaderCxxTests tests
  NO_VALID NO_OUTPUT
  TestCGNSReader.cxx
  TestReadCGNSSolution.cxx
  TestCGNSNoFlowSolutionPointers.cxx
  TestCGNSUnsteadyFields.cxx
  TestCGNSUnsteadyGrid.cxx
  TestCGNSReaderMeshCaching.cxx)
vtk_test_cxx_executable(vtkIOCGNSReaderCxxTests tests)
