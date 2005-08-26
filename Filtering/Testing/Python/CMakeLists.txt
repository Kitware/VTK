INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      SET(tests
        closedSplines
        CSpline
        KSpline
        SelectionLoop
        ImplicitSum
        PerlinNoise 
        )

      CONVERT_TCL_TEST_TO_PY (
        tests
        Filtering)
    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
