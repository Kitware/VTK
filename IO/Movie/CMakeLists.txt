set(classes
  vtkGenericMovieWriter)

if (WIN32)
  # Check if VideoForWindows is supported. This also adds an option
  # VTK_USE_VIDEO_FOR_WINDOWS that is set to ON/OFF by default based on
  # whether VideoForWindows was found.
  include(vtkTestVideoForWindows)

  if (VTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND classes vtkAVIWriter)
  endif ()

  # Check if Media Foundation is supported. This also adds an option
  # VTK_USE_MICROSOFT_MEDIA_FOUNDATION that is set to ON/OFF by default based
  # whether Microsoft Media Foundation was found.
  include(vtkTestMicrosoftMediaFoundation)

  if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
    list(APPEND classes vtkMP4Writer)
  endif ()
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkIOMovieConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h"
  @ONLY)

vtk_module_add_module(VTK::IOMovie
  CLASSES ${classes}
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h")

set(libs)
if (WIN32)
  if (VTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND libs vfw32)
  endif ()

  if (VTK_USE_MICROSOFT_MEDIA_FOUNDATION)
    list(APPEND libs mfreadwrite mfplat mfuuid)
  endif()

  vtk_module_link(VTK::IOMovie
    PRIVATE ${libs})
endif ()
