INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      SET(tests
         dem
         NetCDFCF2DBounds
         NetCDFCFSetOutputType
         NetCDFCFSphericalCoords
         NetCDFCFUnstructured
         NetCDFReader
         TestAVSucdReader
         TestGAMBITReader
         TestImageWriters
         TestPLYReadWrite
         TestPolygonWriters
         TestJPEGReader
         TestTIFFReader
         TestBMPReader
         Plot3DScalars
         Plot3DVectors
         ParticleReader
         caffeine
         TestXYZMol
         )
       IF(VTK_USE_METAIO)
         SET(tests ${tests} TestMHD)
       ENDIF(VTK_USE_METAIO)
       CONVERT_TCL_TEST_TO_PY (
         tests
         IO)

    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
