
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
  ADD_EXECUTABLE(BoxWidget BoxWidget)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(BoxWidget-image ${CXX_TEST_PATH}/BoxWidget
                -D ${VTK_DATA_ROOT}
                -V Baseline/Graphics/BoxWidget.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(BoxWidget ${CXX_TEST_PATH}/BoxWidget)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

LINK_LIBRARIES (
vtkImaging
vtkGraphics
vtkIO
vtkFiltering 
vtkCommon 
)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtktiff )
