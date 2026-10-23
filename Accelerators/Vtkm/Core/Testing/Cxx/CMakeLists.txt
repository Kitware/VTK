# We are splitting the tests into two executables to workaround an issue in
# cuda. With all the tests in the same executable several tests are failing
# in cuda. We have not identified the root cause of the problem yet.
vtk_add_test_cxx(vtkAcceleratorsVTKmCoreCxxTests tests
  TestVTKMDataArray.cxx,NO_VALID
  TestVTKMImplicitDataArray.cxx,NO_VALID
  TestVTKMNewVTKArray.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkAcceleratorsVTKmCoreCxxTests tests)
