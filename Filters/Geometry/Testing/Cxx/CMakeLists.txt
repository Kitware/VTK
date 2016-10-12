vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestExtractSurfaceNonLinearSubdivision.cxx
  TestDataSetSurfaceFieldData.cxx,NO_VALID
  TestDataSetRegionSurfaceFilter.cxx
  TestImageDataToUniformGrid.cxx,NO_VALID
  TestLinearToQuadraticCellsFilter.cxx
  TestProjectSphereFilter.cxx,NO_VALID
  TestStructuredAMRNeighbor.cxx,NO_VALID
  TestUniformGridGhostDataGenerator.cxx,NO_VALID
  TestUnstructuredGridGeometryFilter.cxx
  )
vtk_add_test_cxx(${vtk-module}CxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
  TestGeometryFilterCellData.cxx
  TestStructuredAMRGridConnectivity.cxx
  TestStructuredGridConnectivity.cxx
  TestStructuredGridGhostDataGenerator.cxx
  UnitTestDataSetSurfaceFilter.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests)
