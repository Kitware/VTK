vtk_add_test_cxx(vtkIOOggTheoraCxxTests tests
  NO_VALID
  TestOggTheoraWriter.cxx
  )

vtk_test_cxx_executable(vtkIOOggTheoraCxxTests tests)
