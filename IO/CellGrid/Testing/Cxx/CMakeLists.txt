vtk_add_test_cxx(vtkIOCellGridCxxTests tests
  TestCellGridReadWrite.cxx,NO_VALID
  TestCellGridInheritance.cxx,NO_VALID
)
vtk_test_cxx_executable(vtkIOCellGridCxxTests tests)
