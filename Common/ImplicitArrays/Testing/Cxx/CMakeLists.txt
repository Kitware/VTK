vtk_add_test_cxx(vtkCommonImplicitArrayCxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestAffineArray.cxx
  TestCompositeArray.cxx
  TestCompositeImplicitBackend.cxx
  TestConstantArray.cxx
  TestImplicitArraysBase.cxx
  TestImplicitArrayTraits.cxx
  TestIndexedArray.cxx
  TestIndexedImplicitBackend.cxx
  TestStdFunctionArray.cxx
)

vtk_test_cxx_executable(vtkCommonImplicitArrayCxxTests tests)
