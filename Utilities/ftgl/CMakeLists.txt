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
  ${VTKFTGL_BINARY_DIR}
  ${VTKFREETYPE_SOURCE_DIR}/include
  ${VTKFREETYPE_BINARY_DIR}
)

#
# Do not use the STL (portability issue)
#
SET (VTKFTGL_DO_NOT_USE_STL 1)

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
  src/FTGlyph.cpp
  src/FTGlyphContainer.cpp
  src/FTLibrary.cpp
  src/FTPixmapGlyph.cpp
  src/FTSize.cpp
)

#
# Do not use texture fonts (crash on Type1 fonts)
#
SET (VTKFTGL_DO_NOT_USE_TEXTURE_FONT 1)
IF (NOT VTKFTGL_DO_NOT_USE_TEXTURE_FONT)
  SOURCE_FILES (FTGL_SRCS
    src/FTGLTextureFont.cpp
    src/FTTextureGlyph.cpp
  )
ENDIF (NOT VTKFTGL_DO_NOT_USE_TEXTURE_FONT)

#
# Do not use vector fonts (we only need pixmap and bitmaps and vector fonts
# needs glu). Might be an option at some point, but set it to 'true' for now.
#
SET (VTKFTGL_DO_NOT_USE_VECTORISER 1)
IF (NOT VTKFTGL_DO_NOT_USE_VECTORISER)
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
ENDIF (NOT VTKFTGL_DO_NOT_USE_VECTORISER)

#
# Shared/static lib settings)
#
IF (BUILD_SHARED_LIBS)
  SET (VTKFTGL_DLL 1)
ELSE (BUILD_SHARED_LIBS)
  SET (VTKFTGL_STATIC 1)
ENDIF (BUILD_SHARED_LIBS)

#
# Win32 flags (and shared/static lib settings)
#
IF (WIN32)
  SET (VTKFTGL_WIN32_FLAGS 1) # -DUSE_STD_NAMESPACE and -DWIN32
ENDIF (WIN32)

#
# Unix flags
#
IF (UNIX)
  # IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
  SET (VTKFTGL_HPUX_SOURCE 1)
  # ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
ENDIF (UNIX)

#
# Apple flags
#
IF (APPLE)
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -fpascal-strings")
  SET (CMAKE_CXX_SHLIB_LINK_FLAGS 
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
# Define the library (and install it)
#
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
# Create the configuration file
#
ADD_DEFINITIONS (-DVTKFTGL)
CONFIGURE_FILE(${VTKFTGL_SOURCE_DIR}/vtkftglConfig.h.in
               ${VTKFTGL_BINARY_DIR}/vtkftglConfig.h)

#
# Build examples (testing purposes)
# (Warning, they use Glut)
#
OPTION (FTGL_BUILD_EXAMPLES "Build FTGL examples (warning: you need Glut)" OFF)
MARK_AS_ADVANCED (FTGL_BUILD_EXAMPLES)

IF (FTGL_BUILD_EXAMPLES)

  #
  # Need GLU and GLUT
  #
  INCLUDE (${CMAKE_ROOT}/Modules/FindGLU.cmake)
  INCLUDE (${CMAKE_ROOT}/Modules/FindGLUT.cmake)

  IF (GLU_LIBRARY AND GLUT_LIBRARY)
    IF (GLUT_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLUT_INCLUDE_PATH})
    ENDIF (GLUT_INCLUDE_PATH)
    LINK_LIBRARIES (${GLUT_LIBRARY})

    IF (GLU_INCLUDE_PATH)
      INCLUDE_DIRECTORIES (${GLU_INCLUDE_PATH})
    ENDIF (GLU_INCLUDE_PATH)
    LINK_LIBRARIES (${GLU_LIBRARY})

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
      ADD_DEFINITIONS (-DGLUT_DISABLE_ATEXIT_HACK)
      ADD_DEFINITIONS (-DGLUT_BUILDING_LIB)
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
