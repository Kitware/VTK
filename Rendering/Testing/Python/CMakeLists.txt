IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (PYTHON_EXECUTABLE)
      FOREACH ( tfile
                TestTkRenderWidget
                TestTkRenderWindowInteractor
                )
        ADD_TEST(${tfile}Python-image ${PYTHON_EXECUTABLE}
          ${VTK_SOURCE_DIR}/Rendering/Testing/Python/${tfile}.py
          -B ${VTK_DATA_ROOT}/Baseline/Rendering
          -D ${VTK_DATA_ROOT}/Data)
      ENDFOREACH( tfile )
    ENDIF (PYTHON_EXECUTABLE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

