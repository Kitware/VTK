vtk_add_test_cxx(vtkIOERFCxxTests tests
  TestERFReader.cxx,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkIOERFCxxTests tests)
