#
# Find the native FFMPEG includes and library
#
# This module defines
# FFMPEG_INCLUDE_DIR, where to find avcodec.h, avformat.h ...
# FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
# FFMPEG_FOUND, If false, do not try to use FFMPEG.

# also defined, but not for general use are
# FFMPEG_avformat_LIBRARY and FFMPEG_avcodec_LIBRARY, where to find the FFMPEG library.
# This is useful to do it this way so that we can always add more libraries
# if needed to FFMPEG_LIBRARIES if ffmpeg ever changes...

# If the FFMPEG headers are all in one directory.
find_path(FFMPEG_INCLUDE_DIR avformat.h
  PATHS
    $ENV{FFMPEG_DIR}/include
    $ENV{OSGDIR}/include
    $ENV{OSG_ROOT}/include
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
  PATH_SUFFIXES ffmpeg
  DOC "Location of FFMPEG Headers"
)

# If the FFMPEG headers are separated to each of libavformat, libavcodec etc..
if(NOT FFMPEG_INCLUDE_DIR)
  find_path(FFMPEG_INCLUDE_DIR libavformat/avformat.h
    PATHS
      $ENV{FFMPEG_DIR}/include
      $ENV{OSGDIR}/include
      $ENV{OSG_ROOT}/include
      ~/Library/Frameworks
      /Library/Frameworks
      /usr/local/include
      /usr/include
      /sw/include # Fink
      /opt/local/include # DarwinPorts
      /opt/csw/include # Blastwave
      /opt/include
      /usr/freeware/include
    PATH_SUFFIXES ffmpeg
    DOC "Location of FFMPEG Headers"
  )
endif()

# We want the -I include line to use the parent directory of FFMPEG, it
# uses relative includes such as <ffmpeg/avformat.h> or <libavcodec/avformat.h>
get_filename_component(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} ABSOLUTE)

find_library(FFMPEG_avformat_LIBRARY avformat
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_avcodec_LIBRARY avcodec
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_avutil_LIBRARY avutil
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_vorbis_LIBRARY vorbis
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_dc1394_LIBRARY dc1394_control
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_vorbisenc_LIBRARY vorbisenc
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_theora_LIBRARY theora
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_dts_LIBRARY dts
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_gsm_LIBRARY gsm
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_swscale_LIBRARY swscale
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_z_LIBRARY z
  /usr/local/lib
  /usr/lib
)

unset(FFMPEG_LIBRARIES)
if(FFMPEG_INCLUDE_DIR AND FFMPEG_avformat_LIBRARY AND FFMPEG_avcodec_LIBRARY
  AND FFMPEG_avutil_LIBRARY)
  set(FFMPEG_FOUND TRUE)
  set(FFMPEG_BASIC_LIBRARIES
    ${FFMPEG_avcodec_LIBRARY}
    ${FFMPEG_avformat_LIBRARY}
    ${FFMPEG_avutil_LIBRARY}
  )

  # swscale is always a part of newer ffmpeg distros
  if(FFMPEG_swscale_LIBRARY)
    list(APPEND FFMPEG_BASIC_LIBRARIES ${FFMPEG_swscale_LIBRARY})
  endif()

  set(FFMPEG_LIBRARIES ${FFMPEG_BASIC_LIBRARIES})

  if(FFMPEG_vorbis_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_vorbis_LIBRARY})
  endif()

  if(FFMPEG_dc1394_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_dc1394_LIBRARY})
  endif()

  if(FFMPEG_vorbisenc_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_vorbisenc_LIBRARY})
  endif()

  if(FFMPEG_theora_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_theora_LIBRARY})
  endif()

  if(FFMPEG_dts_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_dts_LIBRARY})
  endif()

  if(FFMPEG_gsm_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_gsm_LIBRARY})
  endif()

  if(FFMPEG_z_LIBRARY)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_z_LIBRARY})
  endif()

  set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} CACHE INTERNAL "All presently found FFMPEG libraries.")

endif()

mark_as_advanced(
  FFMPEG_INCLUDE_DIR
  FFMPEG_avformat_LIBRARY
  FFMPEG_avcodec_LIBRARY
  FFMPEG_avutil_LIBRARY
  FFMPEG_vorbis_LIBRARY
  FFMPEG_dc1394_LIBRARY
  FFMPEG_vorbisenc_LIBRARY
  FFMPEG_theora_LIBRARY
  FFMPEG_dts_LIBRARY
  FFMPEG_gsm_LIBRARY
  FFMPEG_swscale_LIBRARY
  FFMPEG_z_LIBRARY
  )
