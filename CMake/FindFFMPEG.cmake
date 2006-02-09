#
# Find the native FFMPEG includes and library
#
# This module defines
# FFMPEG_INCLUDE_DIR, where to find avcodec.h, avformat.h ...
# FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
# FFMPEG_FOUND, If false, do not try to use FFMPEG.

# also defined, but not for general use are
# FFMPEG_avformat_LIBRARY and FFMPEG_avcodec_LIBRARY, where to find the FFMPEG library.
# This is usefull to do it this way so that we can always add more libraries
# if needed to FFMPEG_LIBRARIES if ffmpeg ever changes...

FIND_PATH(FFMPEG_INCLUDE_DIR ffmpeg/avformat.h
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(FFMPEG_avformat_LIBRARY avformat
  /usr/local/lib
  /usr/lib
)
FIND_LIBRARY(FFMPEG_avcodec_LIBRARY avcodec
  /usr/local/lib
  /usr/lib
)

IF(FFMPEG_INCLUDE_DIR)
  IF(FFMPEG_LIBRARY)
    SET( FFMPEG_FOUND "YES" )
    SET( FFMPEG_LIBRARIES ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avformat_LIBRARY} )
  ENDIF(FFMPEG_LIBRARY)
ENDIF(FFMPEG_INCLUDE_DIR)


