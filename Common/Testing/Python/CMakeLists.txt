#
# Add tests, with the data
#
IF (VTK_PYTHON_EXE)
  FOREACH ( tfile
      PythonSmoke
      )
    ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
      ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py)
  ENDFOREACH( tfile )
ENDIF (VTK_PYTHON_EXE)
