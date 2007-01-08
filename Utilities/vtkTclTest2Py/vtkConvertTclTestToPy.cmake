# Macro to convert tcl tests to python and add those tests.
# Assumes VTK_WRAP_PYTHON is on and PYTHON_EXECUTABLE is defined.
MACRO (CONVERT_TCL_TEST_TO_PY tcl_tests kit_name)
  IF(NOT "${${tcl_tests}}" STREQUAL "")
    SET (input_dir ${VTK_SOURCE_DIR}/${kit_name}/Testing/Tcl)
    SET (output_dir ${VTK_BINARY_DIR}/${kit_name}/Testing/Python)
    SET (target_name ${kit_name}PythonTests)

    SET(CMD ${PYTHON_EXECUTABLE})
    SET (CONVERTED_TESTS)
    SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
    SET (TESTS_TO_CONVERT)
    SET (CONVERSIONLIST)

    FOREACH(test ${${tcl_tests}})
      SET(input "${input_dir}/${test}.tcl")
      SET(output "${output_dir}/${test}.py")

      SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
      SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
      SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")
    
      #Add the py test.
      IF (${VTK_DATA_ROOT})
        ADD_TEST(${test}Python ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${kit_name}/${test}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ELSE (${VTK_DATA_ROOT})
        ADD_TEST(${test}Python ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${kit_name}/${test}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDIF (${VTK_DATA_ROOT})
    ENDFOREACH(test)

    CONFIGURE_FILE(
      ${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkTestsToConvert.in
      ${output_dir}/vtkTestsToConvert
      @ONLY
      )

    ADD_CUSTOM_COMMAND(
      OUTPUT "${output_dir}/conversion_complete"
      COMMAND ${CMD}
      ARGS ${CONVERTER_SCRIPT}
      -l "${output_dir}/vtkTestsToConvert"
      -t "${output_dir}/conversion_complete"
      -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
      -A "${VTK_BINARY_DIR}/Wrapping/Python"
      -A "${VTK_LIBRARY_DIR}"
      DEPENDS ${TESTS_TO_CONVERT} 
      ${output_dir}/vtkTestsToConvert
      ${CONVERTER_SCRIPT}
      COMMENT "Converting Tcl test"
      )
    ADD_CUSTOM_TARGET(${target_name} ALL DEPENDS
      "${output_dir}/conversion_complete")

    ADD_DEPENDENCIES(${target_name} vtktcltest2py_pyc)

    # TODO: add explicit dependency between the vtk{Name}Kit.cmake files and the
    # the test conversion.
  ENDIF(NOT "${${tcl_tests}}" STREQUAL "")
ENDMACRO (CONVERT_TCL_TEST_TO_PY) 
