vtk_add_test_cxx(vtkCommonImplicitArrayCxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestAffineArray.cxx
  TestConstantArray.cxx
  TestImplicitArraysBase.cxx
  TestImplicitArrayTraits.cxx
  TestStdFunctionArray.cxx
)

vtk_test_cxx_executable(vtkCommonImplicitArrayCxxTests tests)
