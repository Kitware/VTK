set(classes
  vtkVideoSource)

if (WIN32)
  include(vtkTestVideoForWindows)
  if (VTK_VFW_SUPPORTS_CAPTURE)
    list(APPEND classes vtkWin32VideoSource)
  endif ()

  if (VTK_USE_MATROX_IMAGING)
    # Include directory needed for vtkMILVideoSource.cxx.
    list(APPEND classes vtkMILVideoSource)
  endif ()
endif ()

vtk_module_add_module(VTK::IOVideo
  CLASSES ${classes})

if (WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  vtk_module_link(VTK::IOVideo
    PRIVATE
      vfw32)
endif ()
