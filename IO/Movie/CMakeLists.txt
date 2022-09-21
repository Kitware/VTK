set(classes
  vtkGenericMovieWriter)

cmake_dependent_option(VTK_USE_VIDEO_FOR_WINDOWS "Enable using Video for Windows (vfw32) for video output (vtkAVIWriter)." OFF
  WIN32 OFF)
mark_as_advanced(VTK_USE_VIDEO_FOR_WINDOWS)

if (VTK_USE_VIDEO_FOR_WINDOWS)
  try_compile(vtk_video_for_windows_works
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp"
    SOURCES "${CMAKE_CURRENT_LIST_DIR}/vtkTestvfw32.cxx"
    LINK_LIBRARIES vfw32
    OUTPUT_VARIABLE _vfw_output)
  mark_as_advanced(vtk_video_for_windows_works)

  if (NOT vtk_video_for_windows_works)
    message(FATAL_ERROR
      "`VTK_USE_VIDEO_FOR_WINDOWS` was requested, but support detection "
      "failed: ${_vfw_output}.")
  endif ()
endif ()

if (VTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND classes vtkAVIWriter)
endif ()

cmake_dependent_option(VTK_USE_MICROSOFT_MEDIA_FOUNDATION "Enable using Microsoft Media Foundation (mfreadwrite) for video input and output." OFF
  WIN32 OFF)
mark_as_advanced(VTK_USE_MICROSOFT_MEDIA_FOUNDATION)

# Check for vfw32 support
if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
  try_compile(vtk_microsoft_media_foundation_works
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp"
    SOURCES "${CMAKE_CURRENT_LIST_DIR}/vtkTestMicrosoftMediaFoundation.cxx"
    LINK_LIBRARIES mfreadwrite mfplat mfuuid
    OUTPUT_VARIABLE _msmf_output)
  mark_as_advanced(vtk_microsoft_media_foundation_works)

  if (NOT vtk_microsoft_media_foundation_works)
    message(FATAL_ERROR
      "`VTK_USE_MICROSOFT_MEDIA_FOUNDATION` was requested, but support "
      "detection failed: ${_msmf_output}.")
  endif ()
endif ()

if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
  list(APPEND classes vtkMP4Writer)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkIOMovieConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h"
  @ONLY)

vtk_module_add_module(VTK::IOMovie
  CLASSES ${classes}
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h")

set(libs)
if (VTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND libs vfw32)
endif ()

if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
  list(APPEND libs mfreadwrite mfplat mfuuid)
endif()

vtk_module_link(VTK::IOMovie
  PRIVATE ${libs})
vtk_add_test_mangling(VTK::IOMovie)
