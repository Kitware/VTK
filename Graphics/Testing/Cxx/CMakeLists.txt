LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
)

LINK_LIBRARIES( png zlib )

IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF (VTK_MANGLE_MESA)
    LINK_LIBRARIES( ${MESA_LIBRARY})
    IF ( MESA_OS_LIBRARY )
      LINK_LIBRARIES(${MESA_OS_LIBRARY})
    ENDIF ( MESA_OS_LIBRARY )
  ENDIF (VTK_MANGLE_MESA)
  IF (CMAKE_HAS_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (CMAKE_HAS_X)

  #
  # Add all the executables that require rendering
  #
  ADD_EXECUTABLE(CellLocator CellLocator)
  ADD_EXECUTABLE(PointLocator PointLocator)
  ADD_EXECUTABLE(Mace Mace)
  ADD_EXECUTABLE(expCos expCos)
  ADD_EXECUTABLE(volProt volProt)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(CellLocator-image ${CXX_TEST_PATH}/CellLocator
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/CellLocator.png)
    ADD_TEST(Mace-image ${CXX_TEST_PATH}/Mace
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/Mace.png)
    ADD_TEST(PointLocator-image ${CXX_TEST_PATH}/PointLocator
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/PointLocator.png)
    ADD_TEST(expCos-image ${CXX_TEST_PATH}/expCos 
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/expCos.png)
    ADD_TEST(volProt-image ${CXX_TEST_PATH}/volProt
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/volProt.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(Mace ${CXX_TEST_PATH}/Mace )
    ADD_TEST(expCos ${CXX_TEST_PATH}/expCos )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)



