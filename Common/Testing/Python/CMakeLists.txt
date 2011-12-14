#
# Add tests, with the data
#
IF (VTK_PYTHON_EXE)
  FOREACH ( tfile
      PythonSmoke
      TestArrayArguments
      TestGhost
      TestMutable
      TestSubClass
      TestTemplates
      TestWeakref
      TestNumpySupport
      TestTerminationCrash
      TestExecuteMethodFinalizeCrash
      TestIgnoreBTX
      )
    ADD_TEST(${tfile}Python ${VTK_PYTHON_EXE}
      ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py)
  ENDFOREACH( tfile )
  IF (VTK_USE_N_WAY_ARRAYS)
    FOREACH ( tfile
      TestStrings
      TestVariant
      TestOperators
      )
      ADD_TEST(${tfile}Python ${VTK_PYTHON_EXE}
        ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py)
    ENDFOREACH ( tfile )
  ENDIF (VTK_USE_N_WAY_ARRAYS)
ENDIF (VTK_PYTHON_EXE)
