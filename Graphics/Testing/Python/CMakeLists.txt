IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (PYTHON_EXECUTABLE)
      FOREACH ( tfile
                Cone
                glyphComb
                streamComb
                tubeComb
                streamSurface
                streamSurface2
                )
        ADD_TEST(${tfile}Python-image ${PYTHON_EXECUTABLE}
          ${VTK_SOURCE_DIR}/Graphics/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -V Baseline/Graphics/${tfile}.png
          -A ${VTK_SOURCE_DIR}/Wrapping/Python
          -A ${LIBRARY_OUTPUT_PATH})
      ENDFOREACH( tfile )
    ENDIF (PYTHON_EXECUTABLE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

