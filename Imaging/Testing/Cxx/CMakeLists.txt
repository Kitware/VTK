IF (VTK_USE_RENDERING)
  LINK_LIBRARIES (vtkRendering)
  IF (VTK_MANGLE_MESA)
    LINK_LIBRARIES( ${MESA_LIBRARY} )
    IF (MESA_OS_LIBRARY)
      LINK_LIBRARIES(${MESA_OS_LIBRARY})
    ENDIF (MESA_OS_LIBRARY)
  ELSE (VTK_MANGLE_MESA)
    IF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
      LINK_LIBRARIES(${MESA_OS_LIBRARY})
    ENDIF (MESA_OS_LIBRARY AND VTK_OPENGL_HAS_OSMESA)
  ENDIF (VTK_MANGLE_MESA)
  LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
  IF (VTK_USE_X)
    LINK_LIBRARIES(-lXt)
    LINK_LIBRARIES(${CMAKE_X_LIBS})
  ENDIF (VTK_USE_X)

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



LINK_LIBRARIES (
vtkIO
vtkImaging
vtkGraphics
vtkFiltering 
vtkCommon 
)

LINK_LIBRARIES( png zlib vtkjpeg)

