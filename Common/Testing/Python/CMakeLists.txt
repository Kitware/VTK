#
# Add tests, with the data
#
IF (VTK_DATA_ROOT)
  IF (VTK_PYTHON_EXE)
    FOREACH ( tfile
        PythonSmoke
        )
      ADD_TEST(${tfile}Python-image ${VTK_PYTHON_EXE}
        ${VTK_SOURCE_DIR}/Common/Testing/Python/${tfile}.py
        -B ${VTK_DATA_ROOT}/Baseline/Common
        -D ${VTK_DATA_ROOT}/Data)
    ENDFOREACH( tfile )
  ENDIF (VTK_PYTHON_EXE)
ENDIF (VTK_DATA_ROOT)

