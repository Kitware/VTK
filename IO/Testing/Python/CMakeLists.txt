IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET (CONVERTED_TESTS)
      SET (CONVERTER_SCRIPT "${VTK_BINARY_DIR}/Wrapping/Python/vtkTclToPyConvertor.py")
      FOREACH( tfile
          dem 
          TestAVSucdReader
          TestGAMBITReader
          TestImageWriters
          TestPLYReadWrite
          TestPolygonWriters
          TestJPEGReader
          TestTIFFReader
          TestBMPReader
          Plot3DScalars
          Plot3DVectors
          ParticleReader
          caffeine
          TestXYZMol
          TestMHD
          )
        
        SET (input "${VTK_SOURCE_DIR}/IO/Testing/Tcl/${tfile}.tcl")
        SET (output "${VTK_BINARY_DIR}/IO/Testing/Python/${tfile}.py")
        SET (CONVERTED_TESTS ${CONVERTED_TESTS} "${output}")
        ADD_CUSTOM_COMMAND(
          OUTPUT "${output}"
          COMMAND ${PYTHON_EXECUTABLE}
          ARGS ${CONVERTER_SCRIPT}
            "-i" "${input}" "-o" "${output}" 
            -A "${VTK_BINARY_DIR}/Wrapping/Python" 
            -A "${VTK_LIBRARY_DIR}"
          DEPENDS "${input}" "${CONVERTER_SCRIPT}"
          COMMENT "Converting Tcl test"
          )

        #Add the py test.
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_BINARY_DIR}/Wrapping/Python/rtImageTest.py
          ${output}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/IO/${tfile}.png
          -A "${VTK_BINARY_DIR}/Wrapping/Python" 
          -A "${VTK_LIBRARY_DIR}"
          )
      ENDFOREACH( tfile ) 
      ADD_CUSTOM_TARGET(IOPythonTests ALL DEPENDS ${CONVERTED_TESTS}) 
      ADD_DEPENDENCIES(IOPythonTests vtkpython_pyc)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
