vtk_add_test_cxx(vtkIOFDSCxxTests tests
  TestFDSReader.cxx,NO_VALID,NO_OUTPUT
  TestFDSReader2.cxx,NO_VALID,NO_OUTPUT
  TestFDSReader3.cxx,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkIOFDSCxxTests tests)
