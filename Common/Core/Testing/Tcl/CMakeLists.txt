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
  add_test(NAME ${vtk-module}Tcl-${test}
         COMMAND ${TCL_TCLSH}
         ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcl
         -A ${VTK_BINARY_DIR}/Wrapping/Tcl)
endforeach()
