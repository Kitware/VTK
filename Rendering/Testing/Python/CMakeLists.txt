IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      IF(NOT VTK_USE_COCOA)
        FOREACH ( tfile
                  TestTkRenderWidget
                  TestTkRenderWindowInteractor
                  )
          ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
            ${VTK_SOURCE_DIR}/Rendering/Testing/Python/${tfile}.py
            -B ${VTK_DATA_ROOT}/Baseline/Rendering
            -D ${VTK_DATA_ROOT}/Data)
        ENDFOREACH( tfile )
      ENDIF(NOT VTK_USE_COCOA)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

