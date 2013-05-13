# Tests that don't produce images
unset(tests)
set(tests
  TestGlobFileNames
  TestSortFileNames
)

foreach( tfile ${tests})
  if(VTK_DATA_ROOT)
    vtk_add_test_tcl(${tfile}.tcl NO_RT)
  endif()
endforeach()
