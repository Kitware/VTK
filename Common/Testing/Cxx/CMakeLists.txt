LINK_LIBRARIES (vtkCommon)

#
# Add all the executables 
#
FOREACH (test 
  ObjectFactory
  otherArrays
  otherEmptyCell
  otherRectilinearGrid
  otherCellBoundaries
  otherCellDerivatives
  otherCellPosition
  otherStructuredGrid
  otherTimerLog
  otherLookupTable
  otherByteSwap
  otherCellArray
  otherCellTypes
  otherFieldData
  quadraticEvaluation
  TestImageIterator
  TestContainers
  TestCxxFeatures
  TestReferenceCountedList
  TestList
  TestMap
  TestString
  )

  ADD_EXECUTABLE(${test} ${test}.cxx)
  ADD_TEST(${test} ${CXX_TEST_PATH}/${test})
ENDFOREACH (test) 
