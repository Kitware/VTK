LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
)

IF (HAVE_PNG)
  LINK_LIBRARIES( ${PNG_LIBRARY} )
  IF (ZLIB_LIBRARY)
    LINK_LIBRARIES(${ZLIB_LIBRARY})
  ENDIF (ZLIB_LIBRARY)
ENDIF (HAVE_PNG)

IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF (CMAKE_HAS_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (CMAKE_HAS_X)

  #
  # Add all the executables that require rendering
  #
  ADD_EXECUTABLE(Mace Mace)
  ADD_EXECUTABLE(expCos expCos)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(Mace-image ${EXECUTABLE_OUTPUT_PATH}/Mace
                -V ${VTK_DATA_ROOT}/Baseline/Graphics/Mace.pgm)
    ADD_TEST(expCos-image ${EXECUTABLE_OUTPUT_PATH}/expCos 
                -V ${VTK_DATA_ROOT}/Baseline/Graphics/expCos.pgm)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(Mace ${EXECUTABLE_OUTPUT_PATH}/Mace)
    ADD_TEST(expCos ${EXECUTABLE_OUTPUT_PATH}/expCos)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

