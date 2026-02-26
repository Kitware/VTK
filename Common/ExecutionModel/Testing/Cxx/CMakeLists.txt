vtk_add_test_cxx(vtkCommonExecutionModelCxxTests tests
  NO_DATA NO_VALID
  TestAbortExecute.cxx
  TestAbortExecuteFromOtherThread.cxx
  TestAbortSMPFilter.cxx
  TestCopyAttributeData.cxx
  TestErrorCode.cxx
  TestForEach.cxx
  TestTemporalHelpers.cxx
  TestImageDataToStructuredGrid.cxx
  TestMetaData.cxx
  TestMultipleInputArrayComponents.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  TestThreadedImageAlgorithmSplitExtent.cxx
  TestTrivialConsumer.cxx
  TestUpdate.cxx
  UnitTestSimpleScalarTree.cxx
  )

vtk_add_test_cxx(vtkCommonExecutionModelCxxTests tests
  NO_VALID
  TestMultiOutputSimpleFilter.cxx
  )

vtk_test_cxx_executable(vtkCommonExecutionModelCxxTests tests)
