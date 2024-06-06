vtk_add_test_cxx(vtkFiltersCellGridCxxTests tests
  TestBasisFunctions.cxx,NO_VALID
  TestCellGridEvaluator.cxx,NO_VALID
  TestCellGridExtractSurface.cxx,NO_VALID
  TestDGCells.cxx,NO_VALID
)
vtk_test_cxx_executable(vtkFiltersCellGridCxxTests tests)
