vtk_add_test_cxx(NO_VALID
  TestArrayDataWriter.cxx
  TestArrayDenormalized.cxx
  TestArraySerialization.cxx
  TestCompress.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
