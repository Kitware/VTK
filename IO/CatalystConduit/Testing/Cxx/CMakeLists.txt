vtk_add_test_cxx(vtkConduitCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectToConduit.cxx
  TestConduitSource.cxx)

vtk_test_cxx_executable(vtkConduitCxxTests tests)
