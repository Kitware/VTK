if(VTK_PYTHON_EXE)
  set(tests
    TestRCalculatorFilter
    )
  foreach(tfile ${tests})
    vtk_add_test_python(${tfile}.py NO_VALID NO_RT NO_OUTPUT NO_DATA)
  endforeach()
else()
  message(FATAL_ERROR "No Python tests added!")
endif()
