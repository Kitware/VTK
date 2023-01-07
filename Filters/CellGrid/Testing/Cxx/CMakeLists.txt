vtk_add_test_cxx(vtkFiltersCellGridCxxTests tests
  TestCellGridExtractSurface.cxx,NO_VALID
  TestDGCells.cxx,NO_VALID
)
vtk_test_cxx_executable(vtkFiltersCellGridCxxTests tests)
