vtk_add_test_cxx(vtkTestingCoreCxxTests tests
  TestErrorObserver.cxx,NO_VALID
  TestDataObjectCompare.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkTestingCoreCxxTests tests)
