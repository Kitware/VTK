if(VTK_PYTHON_EXE)
  foreach(tfile
      TestAMRResampleFilter
      TestAMRExtractLevel
      TestAMRSliceFilter
   )
  vtk_add_test_python(${tfile}.py NO_VALID NO_OUTPUT)
  set_tests_properties(${vtk-module}Python-${tfile} PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  endforeach()
endif()