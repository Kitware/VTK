vtk_add_test_cxx(vtkCommonExecutionModelCxxTests tests
  NO_DATA NO_VALID
  TestCopyAttributeData.cxx
  TestImageDataToStructuredGrid.cxx
  TestMetaData.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  TestThreadedImageAlgorithmSplitExtent.cxx
  TestTrivialConsumer.cxx
  UnitTestSimpleScalarTree.cxx
  )

vtk_add_test_cxx(vtkCommonExecutionModelCxxTests tests
  NO_VALID
  TestMultiOutputSimpleFilter.cxx
  )

vtk_test_cxx_executable(vtkCommonExecutionModelCxxTests tests)
