vtk_add_test_cxx(vtkCommonExecutionModelCxxTests tests
  NO_DATA NO_VALID
  TestAbortExecute.cxx
  TestAbortExecuteFromOtherThread.cxx
  TestAbortSMPFilter.cxx
  TestCopyAttributeData.cxx
  TestForEach.cxx
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
