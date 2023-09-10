vtk_add_test_cxx(vtkFiltersCellGridCxxTests tests
  TestCellGridEvaluator.cxx,NO_VALID
  TestCellGridExtractSurface.cxx,NO_VALID
  TestDGCells.cxx,NO_VALID
)
vtk_test_cxx_executable(vtkFiltersCellGridCxxTests tests)
set_target_properties(vtkFiltersCellGridCxxTests PROPERTIES LINKER_LANGUAGE CXX)
