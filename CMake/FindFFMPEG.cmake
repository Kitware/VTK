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

FIND_LIBRARY(FFMPEG_vorbis_LIBRARY vorbis
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_dc1394_LIBRARY dc1394_control
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_vorbisenc_LIBRARY vorbisenc
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_theora_LIBRARY theora
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_avutil_LIBRARY avutil
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_dts_LIBRARY dts
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_gsm_LIBRARY gsm
  /usr/local/lib
  /usr/lib
)

FIND_LIBRARY(FFMPEG_z_LIBRARY z
  /usr/local/lib
  /usr/lib
)


IF(FFMPEG_INCLUDE_DIR)
  IF(FFMPEG_avcodec_LIBRARY)
    IF(FFMPEG_avformat_LIBRARY)
      SET( FFMPEG_FOUND "YES" )
      SET( FFMPEG_LIBRARIES 
        ${FFMPEG_avcodec_LIBRARY} 
        ${FFMPEG_avformat_LIBRARY} 
        ${FFMPEG_vorbis_LIBRARY} 
        ${FFMPEG_dc1394_LIBRARY} 
        ${FFMPEG_vorbisenc_LIBRARY} 
        ${FFMPEG_theora_LIBRARY} 
        ${FFMPEG_avutil_LIBRARY} 
        ${FFMPEG_dts_LIBRARY} 
        ${FFMPEG_gsm_LIBRARY} 
        ${FFMPEG_z_LIBRARY})
    ENDIF(FFMPEG_avformat_LIBRARY)
  ENDIF(FFMPEG_avcodec_LIBRARY)
ENDIF(FFMPEG_INCLUDE_DIR)

MARK_AS_ADVANCED(
  FFMPEG_INCLUDE_DIR
  FFMPEG_avcodec_LIBRARY
  FFMPEG_avformat_LIBRARY
  FFMPEG_vorbis_LIBRARY
  FFMPEG_dc1394_LIBRARY
  FFMPEG_vorbisenc_LIBRARY
  FFMPEG_theora_LIBRARY
  FFMPEG_avutil_LIBRARY
  FFMPEG_dts_LIBRARY
  FFMPEG_gsm_LIBRARY
  FFMPEG_z_LIBRARY
  )
