IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      FOREACH ( tfile
                cameraWarpedCone
                glyphComb
                streamComb
                tubeComb
                streamSurface
                streamSurface2
                )
        ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
          ${VTK_SOURCE_DIR}/Graphics/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -V Baseline/Graphics/${tfile}.png
          -A ${VTK_SOURCE_DIR}/Wrapping/Python
          -A ${LIBRARY_OUTPUT_PATH})
      ENDFOREACH( tfile )
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

