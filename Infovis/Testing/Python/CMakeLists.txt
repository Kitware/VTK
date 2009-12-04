IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  IF (VTK_PYTHON_EXE)
    SET(tests)
    
    IF (VTK_DATA_ROOT)
      # Tests written in Python that require VTKData
      # These tests should use vtk.test.Testing.
      #SET( tests
      #  ${tests}
      #)
    ELSE (VTK_DATA_ROOT)
      # Tests written in Python that do NOT require VTKData
      # These tests should use vtk.test.Testing.
      SET( tests
        ${tests}
        kMeansDistanceCalculator
      )
    ENDIF (VTK_DATA_ROOT)
    IF ( tests )
      FOREACH ( tfile
                ${tests}
                )
        ADD_TEST(${tfile}Python ${VTK_PYTHON_EXE}
          ${VTK_SOURCE_DIR}/Infovis/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -B ${VTK_DATA_ROOT}/Baseline/Infovis)
      ENDFOREACH( tfile )
    ENDIF ( tests )

  ENDIF (VTK_PYTHON_EXE)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
