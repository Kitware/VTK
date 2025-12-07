vtk_add_test_cxx(vtkIOVDBCxxTests tests
  TestOpenVDBReader.cxx
  TestOpenVDBReaderStream.cxx)

vtk_test_cxx_executable(vtkIOVDBCxxTests tests)
