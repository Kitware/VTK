LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
vtkImaging
vtkGraphics
vtkIO
vtkRendering
)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtktiff)

IF (VTK_USE_FREETYPE)
  LINK_LIBRARIES(vtkftgl vtkfreetype)
ENDIF (VTK_USE_FREETYPE)

LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
IF (VTK_MANGLE_MESA)
  LINK_LIBRARIES( ${MESA_LIBRARY})
  IF ( MESA_OS_LIBRARY )
    LINK_LIBRARIES(${MESA_OS_LIBRARY})
  ENDIF ( MESA_OS_LIBRARY )
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
ADD_EXECUTABLE(TimeRenderer TimeRenderer.cxx)
ADD_EXECUTABLE(TimeRenderer2 TimeRenderer2.cxx)
ADD_EXECUTABLE(otherCoordinate otherCoordinate.cxx)
ADD_EXECUTABLE(TestRenderWindow TestRenderWindow.cxx)

ADD_TEST(otherCoordinate ${CXX_TEST_PATH}/otherCoordinate)

ADD_TEST(TestRenderWindow-image ${CXX_TEST_PATH}/TestRenderWindow
                      -D ${VTK_DATA_ROOT}
                      -FrontBuffer
                      -V Baseline/Rendering/TestRenderWindow.png)






