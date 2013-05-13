set(tests
  TestAmoebaMinimizer
  )

foreach(test ${tests})
  vtk_add_test_tcl(${test}.tcl NO_DATA NO_RT)
endforeach()
