vtk_add_test_cxx(vtkIOCGNSReaderCxxTests tests
  NO_VALID NO_OUTPUT
  TestCGNSFaceMesh.cxx
  TestCGNSNoFlowSolutionPointers.cxx
  TestCGNSReader.cxx
  TestCGNSReaderMeshCaching.cxx
  TestCGNSReaderMissingBase.cxx
  TestCGNSReaderMixedElementNodes.cxx
  TestCGNSReaderUserDefinedData.cxx
  TestCGNSUnsteadyFields.cxx
  TestCGNSUnsteadyGrid.cxx
  TestCGNSUnsteadyTemporalSolution.cxx
  TestReadCGNSSolution.cxx)
vtk_test_cxx_executable(vtkIOCGNSReaderCxxTests tests)
