unset(tests)
set(tests
  TestAmoebaMinimizer
  )

foreach(test ${tests})
  add_test(NAME ${vtk-module}Tcl-${test}
    COMMAND ${VTK_TCL_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcl
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
endforeach()
