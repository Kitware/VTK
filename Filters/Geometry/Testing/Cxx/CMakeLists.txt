vtk_add_test_cxx(vtkFiltersGeometryCxxTests tests
  TestDataSetRegionSurfaceFilter.cxx
  TestDataSetSurfaceFieldData.cxx,NO_VALID
  TestDataSetSurfaceFilterQuadraticTetsGhostCells.cxx,NO_VALID
  TestDataSetSurfaceFilterWith1DGrids.cxx,NO_VALID
  TestDataSetSurfaceFilterWithBlankedImageData.cxx
  TestExplicitStructuredGridSurfaceFilter.cxx
  TestExtractSurfaceNonLinearSubdivision.cxx
  TestImageDataToUniformGrid.cxx,NO_VALID
  TestLinearToQuadraticCellsFilter.cxx
  TestProjectSphereFilter.cxx,NO_VALID
  TestRegularGridDataSetSurfaceFilter.cxx
  TestStructuredAMRNeighbor.cxx,NO_VALID
  TestUnstructuredGridGeometryFilter.cxx
  )
vtk_add_test_cxx(vtkFiltersGeometryCxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
  TestGeometryFilterCellData.cxx
  TestMappedUnstructuredGrid.cxx
  TestStructuredAMRGridConnectivity.cxx
  TestStructuredGridConnectivity.cxx
  UnitTestDataSetSurfaceFilter.cxx
  UnitTestProjectSphereFilter.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_cxx_executable(vtkFiltersGeometryCxxTests all_tests)
