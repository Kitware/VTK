vtk_add_test_cxx(vtkIOFLUENTCFFCxxTests tests
  TestFLUENTCFFReader.cxx,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkIOFLUENTCFFCxxTests tests)
