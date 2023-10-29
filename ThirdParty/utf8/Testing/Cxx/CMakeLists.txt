set(tests
  TestIncludeUTF8.cxx)

vtk_add_test_cxx(vtkutf8CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkutf8CxxTests tests)
