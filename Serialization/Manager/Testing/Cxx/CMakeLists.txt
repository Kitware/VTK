vtk_add_test_cxx(vtkSerializationManagerCxxTests tests
  TestInvoke.cxx,NO_VALID
  TestSession.cxx,NO_DATA
  )
vtk_test_cxx_executable(vtkSerializationManagerCxxTests tests)
