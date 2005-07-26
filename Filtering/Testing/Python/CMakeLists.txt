IF (VTK_USE_RENDERING)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET (CONVERTED_TESTS)
      SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
      SET (TESTS_TO_CONVERT)
      SET (CONVERSIONLIST)
      FOREACH( tfile
          closedSplines
          CSpline
          KSpline
          SelectionLoop
          ImplicitSum
          PerlinNoise 
          )
        
        SET (input "${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/Filtering/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
        SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Filtering/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile ) 
      WRITE_FILE(${VTK_BINARY_DIR}/Filtering/Testing/Python/conversionlist
        "${CONVERSIONLIST}")

      ADD_CUSTOM_COMMAND(
        OUTPUT "${VTK_BINARY_DIR}/Filtering/Testing/Python/conversion_complete"
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CONVERTER_SCRIPT}
          -l "${VTK_BINARY_DIR}/Filtering/Testing/Python/conversionlist"
          -t "${VTK_BINARY_DIR}/Filtering/Testing/Python/conversion_complete"
          -A "${VTK_BINARY_DIR}/Wrapping/Python"
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
        DEPENDS ${TESTS_TO_CONVERT} 
            ${VTK_BINARY_DIR}/Filtering/Testing/Python/conversionlist 
            ${CONVERTER_SCRIPT}
        COMMENT "Converting Tcl test"
        )
      ADD_CUSTOM_TARGET(FilteringPythonTests ALL DEPENDS 
        "${VTK_BINARY_DIR}/Filtering/Testing/Python/conversion_complete") 
      ADD_DEPENDENCIES(FilteringPythonTests vtktcltest2py_pyc)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING)
