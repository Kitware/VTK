vtk_add_test_cxx(vtkIOTecplotTableCxxTests tests
  NO_VALID
  TestTecplotTableReader.cxx
  )

vtk_test_cxx_executable(vtkIOTecplotTableCxxTests tests)
