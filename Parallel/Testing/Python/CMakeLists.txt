#
# Add tests, with the data
#
IF (VTK_PYTHON_EXE)
  # Add test converted from Tcl
  SET (CONVERTED_TESTS)
  SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/vtkTclToPyConvertor.py")
  SET (TESTS_TO_CONVERT)
  SET (CONVERSIONLIST)
  IF (VTK_DATA_ROOT)
    FOREACH( tfile
        RectOutline
        TestExtractVOI
        TestExtractCTHPart
        TestExtrudePiece
        TestPolyDataPieces
        TestUnstructuredPieces
        TestImageStreamer
        TestPImageWriter
        TestCutMaterial
        TestBranchExtentTranslator
        TestPDataSetReaderGrid
        )
      SET (input "${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/${tfile}.tcl")
      SET (output "${VTK_BINARY_DIR}/Parallel/Testing/Python/${tfile}.py")
      SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
      SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
      SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")

      #Add the py test.
      IF (${tfile} MATCHES TestExtractCTHPart)
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Parallel/${tfile}.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ELSE (${tfile} MATCHES TestExtractCTHPart)
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/Parallel/${tfile}.tcl.png
          -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDIF (${tfile} MATCHES TestExtractCTHPart)


    ENDFOREACH( tfile ) 
  ELSE (VTK_DATA_ROOT)
     FOREACH( tfile
        TestPolyDataPieces
        TestCutMaterial
        TestBranchExtentTranslator
        )
      SET (input "${VTK_SOURCE_DIR}/Parallel/Testing/Tcl/${tfile}.tcl")
      SET (output "${VTK_BINARY_DIR}/Parallel/Testing/Python/${tfile}.py")
      SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
      SET (CONVERSIONLIST ${CONVERSIONLIST} "${input};${output}")
      SET (TESTS_TO_CONVERT ${TESTS_TO_CONVERT} "${input}")

      #Add the py test.
      ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
        ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
        ${output}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Parallel/${tfile}.png
        -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
        -A "${VTK_LIBRARY_DIR}"
        )
    ENDFOREACH( tfile )      
  ENDIF (VTK_DATA_ROOT)
  WRITE_FILE(${VTK_BINARY_DIR}/Parallel/Testing/Python/conversionlist
    "${CONVERSIONLIST}")

  ADD_CUSTOM_COMMAND(
    OUTPUT "${VTK_BINARY_DIR}/Parallel/Testing/Python/conversion_complete"
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CONVERTER_SCRIPT}
      -l "${VTK_BINARY_DIR}/Parallel/Testing/Python/conversionlist"
      -t "${VTK_BINARY_DIR}/Parallel/Testing/Python/conversion_complete"
      -A "${VTK_BINARY_DIR}/Wrapping/Python"
      -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py" 
      -A "${VTK_LIBRARY_DIR}"
    DEPENDS ${TESTS_TO_CONVERT} 
        ${VTK_BINARY_DIR}/Parallel/Testing/Python/conversionlist 
        ${CONVERTER_SCRIPT}
    COMMENT "Converting Tcl test"
    )

  ADD_CUSTOM_TARGET(ParallelPythonTests ALL 
      DEPENDS "${VTK_BINARY_DIR}/Parallel/Testing/Python/conversion_complete") 
  ADD_DEPENDENCIES(ParallelPythonTests vtktcltest2py_pyc)

ENDIF (VTK_PYTHON_EXE)
