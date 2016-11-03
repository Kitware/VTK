vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestArrayDataWriter.cxx
  TestArrayDenormalized.cxx
  TestArraySerialization.cxx
  TestCompressLZ4.cxx
  TestCompressZLib.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
