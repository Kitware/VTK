PROJECT (VTKFTGL)

INCLUDE_REGULAR_EXPRESSION(".*")

INCLUDE_DIRECTORIES (
  ${VTKFTGL_SOURCE_DIR}/src
  ${VTK_SOURCE_DIR}/Utilities/freetype/include
)

SOURCE_FILES (FTGL_SRCS
  src/FTBitmapGlyph.cpp
  src/FTCharmap.cpp
  src/FTExtrdGlyph.cpp
  src/FTFace.cpp
  src/FTFont.cpp
  src/FTGLBitmapFont.cpp
  src/FTGLExtrdFont.cpp
  src/FTGLOutlineFont.cpp
  src/FTGLPixmapFont.cpp
  src/FTGLPolygonFont.cpp
  src/FTGLTextureFont.cpp
  src/FTGlyph.cpp
  src/FTGlyphContainer.cpp
  src/FTLibrary.cpp
  src/FTOutlineGlyph.cpp
  src/FTPixmapGlyph.cpp
  src/FTPolyGlyph.cpp
  src/FTSize.cpp
  src/FTTextureGlyph.cpp
  src/FTVectoriser.cpp
  src/NoSTL/FTCharmapInternal.cpp
)

ADD_DEFINITIONS(-DFTGL_DO_NOT_USE_STL)

IF (WIN32)
  ADD_DEFINITIONS(-DUSE_STD_NAMESPACE)
  ADD_DEFINITIONS(-DWIN32)
  IF (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DFTGL_LIBRARY)
  ELSE (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DFTGL_LIBRARY_STATIC)
  ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

IF (UNIX)
  # IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  # ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
ENDIF (UNIX)

IF (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -fpascal-strings")
  SET(CMAKE_CXX_SHLIB_LINK_FLAGS "${CMAKE_CXX_SHLIB_LINK_FLAGS} -multiply_defined suppress -framework Carbon")
ENDIF (APPLE)

INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
IF (OPENGL_INCLUDE_PATH)
  INCLUDE_DIRECTORIES (${OPENGL_INCLUDE_PATH})
ENDIF (OPENGL_INCLUDE_PATH)
LINK_LIBRARIES (${OPENGL_LIBRARY})

INCLUDE (${CMAKE_ROOT}/Modules/FindGLU.cmake)
IF (GLU_INCLUDE_PATH)
  INCLUDE_DIRECTORIES (${GLU_INCLUDE_PATH})
ENDIF (GLU_INCLUDE_PATH)
LINK_LIBRARIES (${GLU_LIBRARY})

ADD_LIBRARY (vtkftgl FTGL_SRCS)
TARGET_LINK_LIBRARIES(vtkftgl
  vtkfreetype
)

CONFIGURE_FILE (${VTKFTGL_SOURCE_DIR}/.NoDartCoverage
                ${VTKFTGL_BINARY_DIR}/.NoDartCoverage)

INSTALL_TARGETS (/lib/vtk vtkftgl)

IF (BUILD_EXAMPLES)

  INCLUDE (${CMAKE_ROOT}/Modules/FindGLUT.cmake)

  IF (GLUT_LIBRARY)
    IF (GLUT_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLUT_INCLUDE_PATH})
    ENDIF (GLUT_INCLUDE_PATH)
    LINK_LIBRARIES (${GLUT_LIBRARY})

    IF (VTK_USE_X)
      LINK_LIBRARIES (-lXt)
      FIND_LIBRARY (XMU_LIBRARY Xmu
        /usr/lib 
        /usr/local/lib 
        /opt/graphics/OpenGL/lib 
        /usr/openwin/lib 
        /usr/X11R6/lib
        /usr/contrib/X11R6/lib
      )
      MARK_AS_ADVANCED (XMU_LIBRARY)
      IF (XMU_LIBRARY)
        LINK_LIBRARIES (${XMU_LIBRARY})
      ENDIF (XMU_LIBRARY)
      LINK_LIBRARIES(${CMAKE_X_LIBS})
    ENDIF (VTK_USE_X)

    # The following is sort-of a hack to avoid the bug in glut.h
    # Ambiguity between 'std::exit(int)' and 'std::exit(int)' in function...
    IF (BORLAND)
      ADD_DEFINITIONS(-DGLUT_BUILDING_LIB)
    ENDIF (BORLAND)

    IF (APPLE)
      SET(CMAKE_CXX_SHLIB_LINK_FLAGS "${CMAKE_CXX_SHLIB_LINK_FLAGS} -framework Cocoa")
    ENDIF (APPLE)

    LINK_LIBRARIES (
      vtkftgl
      vtkfreetype
    )

    ADD_DEFINITIONS(-DFTGL_NO_TEXTURE_FONTS)

    SOURCE_FILES (FTGL_DEMO_SRCS
      demo/tb.c
      demo/trackball.c
      demo/FTGLDemo.cpp
    )
    ADD_EXECUTABLE (ftgl_demo FTGL_DEMO_SRCS)

    ADD_EXECUTABLE (ftgl_demo2 demo/demo.cpp)

  ENDIF (GLUT_LIBRARY)

ENDIF (BUILD_EXAMPLES)



