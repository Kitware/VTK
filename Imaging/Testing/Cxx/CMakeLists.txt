LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
)

LINK_LIBRARIES( png zlib)

IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
  IF (VTK_MANGLE_MESA)
    LINK_LIBRARIES( ${MESA_LIBRARY} ${MESA_OS_LIBRARY})
  ENDIF (VTK_MANGLE_MESA)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF (CMAKE_HAS_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (CMAKE_HAS_X)

  #
  # Add all the executables that require rendering
  #
  ADD_EXECUTABLE(ImportExport ImportExport)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(ImportExport-image ${CXX_TEST_PATH}/ImportExport
                -D ${VTK_DATA_ROOT}
                -V Baseline/Imaging/ImportExport.png)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)



