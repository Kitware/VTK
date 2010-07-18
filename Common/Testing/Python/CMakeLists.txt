#
# Add tests, with the data
#
IF (VTK_PYTHON_EXE)
  FOREACH ( tfile
      PythonSmoke
      TestWeakref
      TestNumpySupport
      TestTerminationCrash
      TestExecuteMethodFinalizeCrash
      )
    ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
      ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py)
  ENDFOREACH( tfile )
  IF (VTK_USE_N_WAY_ARRAYS)
    FOREACH ( tfile
      TestStrings
      TestVariant
      )
      ADD_TEST(${tfile}Python ${VTK_PYTHON_EXE}
        ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py)
    ENDFOREACH ( tfile )
  ENDIF (VTK_USE_N_WAY_ARRAYS)
ENDIF (VTK_PYTHON_EXE)
