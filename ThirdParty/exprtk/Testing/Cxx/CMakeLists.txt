set(tests
  TestIncludeExprTK.cxx)

vtk_add_test_cxx(vtkexprtkCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkexprtkCxxTests tests)
