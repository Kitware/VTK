PROJECT (VTKFTGL)

#
# Dependency mask
#
INCLUDE_REGULAR_EXPRESSION(".*")

#
# Include path
#
INCLUDE_DIRECTORIES (
  ${VTKFTGL_SOURCE_DIR}/src
  ${VTK_SOURCE_DIR}/Utilities/freetype/include
)

#
# Do not use the STL (portability issue)
#
SET (FTGL_FLAGS_TEMP "-DFTGL_DO_NOT_USE_STL")

#
# Do not use texture fonts (crash on Type1 fonts)
#
SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -DFTGL_DO_NOT_USE_TEXTURE_FONT")

#
# Source files
#
SOURCE_FILES (FTGL_SRCS
  src/FTBitmapGlyph.cpp
  src/FTCharmap.cpp
  src/FTFace.cpp
  src/FTFont.cpp
  src/FTGLBitmapFont.cpp
  src/FTGLPixmapFont.cpp
  src/FTGLTextureFont.cpp
  src/FTGlyph.cpp
  src/FTGlyphContainer.cpp
  src/FTLibrary.cpp
  src/FTPixmapGlyph.cpp
  src/FTSize.cpp
  src/FTTextureGlyph.cpp
)

#
# Do not use vector fonts (we only need pixmap and bitmaps and vector fonts
# needs glu). Might be an option at some point, but set it to 'true' for now.
#
SET (FTGL_DO_NOT_USE_VECTORISER 1)

IF (FTGL_DO_NOT_USE_VECTORISER)
  SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -DFTGL_DO_NOT_USE_VECTORISER")
ELSE (FTGL_DO_NOT_USE_VECTORISER)
  SOURCE_FILES (FTGL_SRCS
    src/FTExtrdGlyph.cpp
    src/FTGLExtrdFont.cpp
    src/FTGLOutlineFont.cpp
    src/FTGLPolygonFont.cpp
    src/FTOutlineGlyph.cpp
    src/FTPolyGlyph.cpp
    src/FTVectoriser.cpp
  )
  INCLUDE (${CMAKE_ROOT}/Modules/FindGLU.cmake)
  IF (GLU_INCLUDE_PATH)
    INCLUDE_DIRECTORIES (${GLU_INCLUDE_PATH})
  ENDIF (GLU_INCLUDE_PATH)
  LINK_LIBRARIES (${GLU_LIBRARY})
ENDIF (FTGL_DO_NOT_USE_VECTORISER)

#
# Win32 flags (and shared/static lib settings)
#
IF (WIN32)
  SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -DUSE_STD_NAMESPACE -DWIN32")
  IF (BUILD_SHARED_LIBS)
    SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -DFTGL_LIBRARY")
  ELSE (BUILD_SHARED_LIBS)
    SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -DFTGL_LIBRARY_STATIC")
  ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

#
# Unix flags
#
IF (UNIX)
  # IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
  SET (FTGL_FLAGS_TEMP "${FTGL_FLAGS_TEMP} -D_HPUX_SOURCE")
  # ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
ENDIF (UNIX)

#
# Apple flags
#
IF (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -fpascal-strings")
  SET(CMAKE_CXX_SHLIB_LINK_FLAGS 
  "${CMAKE_CXX_SHLIB_LINK_FLAGS} -multiply_defined suppress -framework Carbon")
ENDIF (APPLE)

#
# We need OpenGL
#
INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
IF (OPENGL_INCLUDE_PATH)
  INCLUDE_DIRECTORIES (${OPENGL_INCLUDE_PATH})
ENDIF (OPENGL_INCLUDE_PATH)
LINK_LIBRARIES (${OPENGL_LIBRARY})

#
# FTGL Flags (will be used later in Rendering)
#
SET (FTGL_FLAGS "${FTGL_FLAGS_TEMP}" CACHE STRING "FTGL flags.")
MARK_AS_ADVANCED (FTGL_FLAGS)

#
# Define the library (and install it)
#
ADD_DEFINITIONS(${FTGL_FLAGS})
ADD_LIBRARY (vtkftgl FTGL_SRCS)
TARGET_LINK_LIBRARIES(vtkftgl
  vtkfreetype
)
INSTALL_TARGETS (/lib/vtk vtkftgl)

#
# Do not cover this lib
#
CONFIGURE_FILE (${VTKFTGL_SOURCE_DIR}/.NoDartCoverage
                ${VTKFTGL_BINARY_DIR}/.NoDartCoverage)

#
# Build examples (testing purposes)
# (Warning, they use Glut)
#
OPTION (FTGL_BUILD_EXAMPLES "Build FTGL examples (warning: you need Glut)" OFF)
MARK_AS_ADVANCED (FTGL_BUILD_EXAMPLES)

IF (FTGL_BUILD_EXAMPLES)

  INCLUDE (${CMAKE_ROOT}/Modules/FindGLUT.cmake)
  IF (GLUT_LIBRARY)
    IF (GLUT_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLUT_INCLUDE_PATH})
    ENDIF (GLUT_INCLUDE_PATH)
    LINK_LIBRARIES (${GLUT_LIBRARY})

    #
    # Do we need X ?
    #
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

    #
    # Win32 flags
    #
	IF (WIN32)
      # - Avoid the glutCreateMenu_ATEXIT_HACK pb.
      # - Sort-of a hack to avoid the bug in glut.h to avoid
      #   ambiguity between 'std::exit(int)' and 'std::exit(int)' in function
      ADD_DEFINITIONS("-DGLUT_DISABLE_ATEXIT_HACK")
      ADD_DEFINITIONS("-DGLUT_BUILDING_LIB")
	ENDIF (WIN32)

    #
    # Apple flags
    #
    IF (APPLE)
      SET (CMAKE_CXX_SHLIB_LINK_FLAGS 
      "${CMAKE_CXX_SHLIB_LINK_FLAGS} -framework Cocoa")
    ENDIF (APPLE)

    #
    # Link examples to FTGL and Freetype
    #
    LINK_LIBRARIES (
      vtkftgl
      vtkfreetype
    )

    #
    # Example 1
    #
    SOURCE_FILES (FTGL_DEMO_SRCS
      demo/tb.c
      demo/trackball.c
      demo/FTGLDemo.cpp
    )
    ADD_EXECUTABLE (ftgl_demo FTGL_DEMO_SRCS)

    #
    # Example 2
    #
    ADD_EXECUTABLE (ftgl_demo2 demo/demo.cpp)

  ENDIF (GLUT_LIBRARY)

ENDIF (FTGL_BUILD_EXAMPLES)
