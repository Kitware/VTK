vtk_add_test_cxx(vtkIOCGNSReaderCxxTests tests
  NO_VALID NO_OUTPUT
  TestCGNSFaceMesh.cxx
  TestCGNSNoFlowSolutionPointers.cxx
  TestCGNSReader.cxx
  TestCGNSReaderBCDirichlet.cxx
  TestCGNSReaderBCNeumann.cxx
  TestCGNSReaderHOElements.cxx
  TestCGNSReaderIgnoreMesh.cxx
  TestCGNSReaderMeshCaching.cxx
  TestCGNSReaderMissingBase.cxx
  TestCGNSReaderMixedElementNodes.cxx
  TestCGNSReaderUserDefinedData.cxx
  TestCGNSUnsteadyFields.cxx
  TestCGNSUnsteadyGrid.cxx
  TestCGNSUnsteadyTemporalSolution.cxx
  TestCGNSReaderPatchesAsElementNodes.cxx
  TestReadCGNSSolution.cxx
  TestCONVERGECFDCGNSReader.cxx)
vtk_test_cxx_executable(vtkIOCGNSReaderCxxTests tests)
