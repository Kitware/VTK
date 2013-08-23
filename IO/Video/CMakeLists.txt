set(Module_SRCS
  vtkVideoSource.cxx
)

if(WIN32)
  include(vtkTestVideoForWindows)
  if(VTK_VFW_SUPPORTS_CAPTURE)
    list(APPEND Module_SRCS vtkWin32VideoSource.cxx)
  endif()

  if(VTK_USE_MATROX_IMAGING)
    list(APPEND Module_SRCS vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  endif()
endif()

vtk_module_library(vtkIOVideo ${Module_SRCS})


if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  target_link_libraries(vtkIOVideo LINK_PRIVATE vfw32)
endif()
