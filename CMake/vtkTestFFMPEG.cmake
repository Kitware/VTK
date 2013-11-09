if(FFMPEG_INCLUDE_DIR)
  if(NOT DEFINED VTK_FFMPEG_HAS_OLD_HEADER)
    if(EXISTS ${FFMPEG_INCLUDE_DIR}/ffmpeg)
      set(VTK_FFMPEG_HAS_OLD_HEADER "TRUE" CACHE INTERNAL
        "Is the FFMPEG include in the old location")
    else()
      set(VTK_FFMPEG_HAS_OLD_HEADER "FALSE" CACHE INTERNAL
        "Is the FFMPEG include in the old location")
    endif()
    if(VTK_FFMPEG_HAS_OLD_HEADER)
      message(STATUS "Checking if FFMPEG uses old style header files - yes")
    else()
      message(STATUS "Checking if FFMPEG uses old style header files - no")
    endif()
  endif()

  if(VTK_FFMPEG_HAS_OLD_HEADER)
    set(FFMEG_CODEC_HEADER_PATH "ffmpeg")
    set(FFMEG_FORMAT_HEADER_PATH "ffmpeg")
  else()
    set(FFMEG_CODEC_HEADER_PATH "libavcodec")
    set(FFMEG_FORMAT_HEADER_PATH "libavformat")
  endif()

  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${FFMPEG_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${FFMPEG_avformat_LIBRARY}
    ${FFMPEG_avutil_LIBRARY} ${FFMPEG_avcodec_LIBRARY})

  if(NOT DEFINED VTK_FFMPEG_HAS_IMG_CONVERT AND FFMPEG_avcodec_LIBRARY)
    set(_source "
#include <${FFMEG_CODEC_HEADER_PATH}/avcodec.h>
int main()
{
  img_convert(0, PIX_FMT_RGB24,
              0, PIX_FMT_RGB24,
              0, 0);
  return 0;
}\n")
    check_c_source_compiles("${_source}" VTK_FFMPEG_HAS_IMG_CONVERT)
  endif()

  if(NOT DEFINED VTK_FFMPEG_NEW_ALLOC AND FFMPEG_avformat_LIBRARY)
    set(_source "
#include <${FFMEG_FORMAT_HEADER_PATH}/avformat.h>
int main()
{
  avformat_alloc_context();
  return 0;
}\n")
    check_c_source_compiles("${_source}" VTK_FFMPEG_NEW_ALLOC)
  endif()
  if(NOT DEFINED VTK_FFMPEG_AVCODECID AND FFMPEG_avformat_LIBRARY)
    set(_source "
#include <${FFMEG_FORMAT_HEADER_PATH}/avformat.h>
int main()
{
  enum AVCodecID codec;
  return 0;
}\n")
    check_c_source_compiles("${_source}" VTK_FFMPEG_AVCODECID)
  endif()
endif()
