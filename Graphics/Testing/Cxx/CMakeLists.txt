
IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF (VTK_MANGLE_MESA)
    IF(NOT MESA_LIBRARY)
      MESSAGE("VTK_MANGLE_MESA option requires the path to MESA_LIBRARY to be specified, please edit the cache.")
    ELSE(NOT MESA_LIBRARY)
      LINK_LIBRARIES( ${MESA_LIBRARY})
    ENDIF(NOT MESA_LIBRARY)
  ELSE (VTK_MANGLE_MESA)
    IF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
      LINK_LIBRARIES(${MESA_OS_LIBRARY})
    ENDIF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
  ENDIF (VTK_MANGLE_MESA)
  IF (VTK_USE_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (VTK_USE_X)

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
    ADD_TEST(CellLocator ${CXX_TEST_PATH}/CellLocator)
    ADD_TEST(PointLocator ${CXX_TEST_PATH}/PointLocator)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

LINK_LIBRARIES (
vtkImaging
vtkGraphics
vtkIO
vtkFiltering 
vtkCommon 
)

LINK_LIBRARIES( png zlib vtkjpeg)
