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
  ADD_EXECUTABLE(ImportExport ImportExport)
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
    ADD_TEST(ImportExport-image ${CXX_TEST_PATH}/ImportExport
                -D ${VTK_DATA_ROOT}
                -V Baseline/Imaging/ImportExport.png)
  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)

ELSE (HAVE_PNG)
#
# if we dont have PNG support we can still run some tests
#
 IF (VTK_USE_RENDERING)
  IF (VTK_DATA_ROOT)
    ADD_TEST(ImportExport ${CXX_TEST_PATH}/ImportExport)
  ENDIF (VTK_DATA_ROOT)
 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)


