set(classes
  vtkVideoSource)

set(vfw_capture_default OFF)
if (DEFINED VTK_USE_VFW_CAPTURE)
  set(vfw_capture_default "${VTK_USE_VFW_CAPTURE}")
  unset(CACHE VTK_USE_VFW_CAPTURE)
endif ()

cmake_dependent_option(VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE "Enable using Video for Windows (vfw32) for video input." "${vfw_capture_default}"
  "WIN32" OFF)
mark_as_advanced(VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE)

# Check if vfw32 supports the video capture functions
if (VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE)
  try_compile(vtk_vfw_capture_works
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp"
    SOURCES "${CMAKE_CURRENT_LIST_DIR}/vtkTestvfw32Capture.cxx"
    LINK_LIBRARIES vfw32
    OUTPUT_VARIABLE _vfw_capture_output)
  mark_as_advanced(vtk_vfw_capture_works)

  if (NOT vtk_vfw_capture_works)
    message(FATAL_ERROR
      "`VTK_USE_VFW_CAPTURE` was requested, but support detection failed: ${_vfw_capture_output}.")
  endif ()
endif ()

if (VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE)
  list(APPEND classes vtkWin32VideoSource)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkIOVideoConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkIOVideoConfigure.h"
  @ONLY)

vtk_module_add_module(VTK::IOVideo
  CLASSES ${classes}
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/vtkIOVideoConfigure.h")

if (WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  vtk_module_link(VTK::IOVideo
    PRIVATE
      vfw32)
endif ()
vtk_add_test_mangling(VTK::IOVideo)
