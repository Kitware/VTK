IF (HAVE_PNG)
 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (PYTHON_EXECUTABLE)
      ADD_TEST(MacePython-image ${PYTHON_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Graphics/Testing/Python/Cone.py
        -D ${VTK_DATA_ROOT}
        -V Baseline/Graphics/Cone.png)
    ENDIF (PYTHON_EXECUTABLE)
  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)

