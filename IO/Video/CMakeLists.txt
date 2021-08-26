set(classes
  vtkVideoSource)

if (WIN32)
  include(vtkTestVideoForWindows)
  if (VTK_VFW_SUPPORTS_CAPTURE)
    list(APPEND classes vtkWin32VideoSource)
  endif ()
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
