#
# Add tests
#
INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)

IF (VTK_PYTHON_EXE AND VTK_USE_DISPLAY)
  # Add tests converted from Tcl
  SET (tests)
  IF (VTK_DATA_ROOT)
    SET( tests ${tests}
      RectOutline
      TestExtractVOI
      TestExtractCTHPart
      TestExtrudePiece
      TestPolyDataPieces
      TestUnstructuredPieces
      TestImageStreamer
      TestPImageWriter
      TestCutMaterial
      TestBranchExtentTranslator
      TestPDataSetReaderGrid
      )
  ELSE (VTK_DATA_ROOT)
      # All Parallel tests require VTKData - no tests should be in this block...
  ENDIF (VTK_DATA_ROOT)

  CONVERT_TCL_TEST_TO_PY (
    tests
    Parallel)
ENDIF (VTK_PYTHON_EXE AND VTK_USE_DISPLAY)
