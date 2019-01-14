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
endif ()

vtk_module_add_module(VTK::IOMovie
  CLASSES ${classes})

if (WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  vtk_module_link(VTK::IOMovie
    PRIVATE
      vfw32)
endif ()
