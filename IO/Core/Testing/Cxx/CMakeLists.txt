set(extra_tests)
if ((NOT DEFINED MSVC_VERSION) OR (MSVC_VERSION GREATER 1800))
  # skip this test on MSVC 2013 or older.
  set(extra_tests
    TestNumberToString.cxx)
endif()

vtk_add_test_cxx(vtkIOCoreCxxTests tests
  NO_VALID
  TestArrayDataWriter.cxx
  TestArrayDenormalized.cxx
  TestArraySerialization.cxx
  TestCompressLZ4.cxx
  TestCompressZLib.cxx
  TestCompressLZMA.cxx
  ${extra_tests}
  )
vtk_test_cxx_executable(vtkIOCoreCxxTests tests)
