vtk_module_find_package(
  PACKAGE FFMPEG
  VERSION 2.3.3
  FORWARD_VERSION_REQ MINOR
  COMPONENTS
    avformat
    avcodec
    avutil
    swscale
  OPTIONAL_COMPONENTS
    swresample)

set(classes
  vtkFFMPEGWriter)

set_source_files_properties(vtkFFMPEGWriter.cxx
  PROPERTIES
    COMPILE_DEFINITIONS __STDC_CONSTANT_MACROS)

# vtkFFMPEGVideoSource requires ffmpeg 3.1 or later.
if (NOT FFMPEG_VERSION VERSION_LESS "3.1")
  list(APPEND classes
    vtkFFMPEGVideoSource)
endif ()

set(ffmpeg_libraries)
if (NOT FFMPEG_VERSION VERSION_LESS "5.0")
  if (NOT FFMPEG_swresample_FOUND)
    message(FATAL_ERROR
      "FFMPEG 5.0 requires the `swresample` library.")
  endif ()

  list(APPEND ffmpeg_libraries
    FFMPEG::swresample)
endif ()

vtk_module_add_module(VTK::IOFFMPEG
  CLASSES ${classes})
vtk_module_link(VTK::IOFFMPEG
  PRIVATE
    FFMPEG::avformat
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale
    ${ffmpeg_libraries})
vtk_add_test_mangling(VTK::IOFFMPEG)
