PROJECT(PNG)

# source files for png
SOURCE_FILES(PNG_SRCS
pngget.c    pngrio.c    pngwrite.c
png.c       pngmem.c    pngrtran.c  pngtrans.c  pngwtran.c
pngerror.c  pngpread.c  pngrutil.c  pngvcrd.c   pngwutil.c
pnggccrd.c  pngread.c   pngset.c   pngwio.c
)

CONFIGURE_FILE(
${PNG_SOURCE_DIR}/.NoDartCoverage 
${PNG_BINARY_DIR}/.NoDartCoverage)

IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DPNG_BUILD_DLL -DZLIB_DLL -DPNG_NO_MODULEDEF)
  ELSE(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DPNG_STATIC)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

ADD_LIBRARY(png PNG_SRCS)
TARGET_LINK_LIBRARIES(png zlib)
