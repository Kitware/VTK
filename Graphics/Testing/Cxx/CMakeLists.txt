LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
)

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
  ADD_EXECUTABLE(volProt volProt)
ENDIF (VTK_USE_RENDERING)


IF (HAVE_PNG)
  LINK_LIBRARIES( ${PNG_LIBRARY} )
  IF (ZLIB_LIBRARY)
    LINK_LIBRARIES(${ZLIB_LIBRARY})
  ENDIF (ZLIB_LIBRARY)

 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(Mace-image ${CXX_TEST_PATH}/Mace
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/Mace.png)
    ADD_TEST(expCos-image ${CXX_TEST_PATH}/expCos 
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/expCos.png)
    ADD_TEST(volProt-image ${CXX_TEST_PATH}/volProt
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/volProt.png)
  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)

ELSE (HAVE_PNG)
#
# if we dont have PNG support we can still run some tests
#
 IF (VTK_USE_RENDERING)
  IF (VTK_DATA_ROOT)
    ADD_TEST(volProt ${CXX_TEST_PATH}/volProt)
  ENDIF (VTK_DATA_ROOT)
  ADD_TEST(Mace ${CXX_TEST_PATH}/Mace)
  ADD_TEST(expCos ${CXX_TEST_PATH}/expCos)
 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)


