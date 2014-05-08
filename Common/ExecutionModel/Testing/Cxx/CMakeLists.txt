vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID
  TestCopyAttributeData.cxx
  TestImageDataToStructuredGrid.cxx
  TestMetaData.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
