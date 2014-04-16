if(VTK_PYTHON_EXE)
  set(python_tests
    TestAMRResampleFilter
    TestAMRExtractLevel
    TestAMRSliceFilter
    )
  set(test_files)
  foreach(test IN LISTS python_tests)
    list(APPEND test_files
      ${test}.py)
  endforeach()
  vtk_add_test_python(
    NO_VALID NO_OUTPUT
    ${test_files}
    )
  foreach(test IN LISTS python_tests)
    set_tests_properties(${vtk-module}Python-${test}
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "Error"
    )
  endforeach()
endif()
