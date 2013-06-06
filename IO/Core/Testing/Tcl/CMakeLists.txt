# Tests that don't produce images
unset(tests)
set(tests
  TestGlobFileNames
  TestSortFileNames
)

foreach( tfile ${tests})
  vtk_add_test_tcl(${tfile}.tcl NO_RT)
endforeach()
