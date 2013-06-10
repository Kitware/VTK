vtk_add_test_cxx(NO_DATA NO_VALID
  TestCopyAttributeData.cxx
  TestImageDataToStructuredGrid.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
