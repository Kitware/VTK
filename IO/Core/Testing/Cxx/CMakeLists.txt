vtk_add_test_cxx(vtkIOCoreCxxTests tests
  NO_VALID
  TestArrayDataWriter.cxx
  TestArrayDenormalized.cxx
  TestArraySerialization.cxx
  TestCompressLZ4.cxx
  TestCompressZLib.cxx
  TestCompressLZMA.cxx
  TestResourceParser.cxx
  TestResourceStreams.cxx
  TestURI.cxx
  TestURILoader.cxx
  )
vtk_test_cxx_executable(vtkIOCoreCxxTests tests)
