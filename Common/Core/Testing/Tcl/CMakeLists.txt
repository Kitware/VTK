set(tests
  TestArrays
  TestEmptyInput
  TestSetGet
  TestString
  TestIgnoreBTX
  otherInterp
  otherPrint
  )

foreach(test ${tests})
  vtk_add_test_tcl(${test}.tcl NO_DATA NO_RT)
endforeach()
