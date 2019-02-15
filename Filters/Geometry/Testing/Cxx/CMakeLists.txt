vtk_add_test_cxx(vtkFiltersGeometryCxxTests tests
  TestExtractSurfaceNonLinearSubdivision.cxx
  TestDataSetSurfaceFieldData.cxx,NO_VALID
  TestDataSetSurfaceFilterQuadraticTetsGhostCells.cxx,NO_VALID
  TestDataSetSurfaceFilterWith1DGrids.cxx,NO_VALID
  TestDataSetRegionSurfaceFilter.cxx
  TestExplicitStructuredGridSurfaceFilter.cxx
  TestImageDataToUniformGrid.cxx,NO_VALID
  TestLinearToQuadraticCellsFilter.cxx
  TestProjectSphereFilter.cxx,NO_VALID
  TestStructuredAMRNeighbor.cxx,NO_VALID
  TestUniformGridGhostDataGenerator.cxx,NO_VALID
  TestUnstructuredGridGeometryFilter.cxx
  TestUnstructuredGridGeometryFilterGhostCells.cxx,NO_VALID
  )
vtk_add_test_cxx(vtkFiltersGeometryCxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
  TestGeometryFilterCellData.cxx
  TestStructuredAMRGridConnectivity.cxx
  TestStructuredGridConnectivity.cxx
  TestStructuredGridGhostDataGenerator.cxx
  UnitTestDataSetSurfaceFilter.cxx
  UnitTestProjectSphereFilter.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_cxx_executable(vtkFiltersGeometryCxxTests all_tests)
