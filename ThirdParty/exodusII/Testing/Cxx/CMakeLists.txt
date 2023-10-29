set(tests
  TestIncludeExodusII.cxx)

vtk_add_test_cxx(vtkexodusIICxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkexodusIICxxTests tests)
