set(Module_SRCS
  vtkVideoSource.cxx
)

if(WIN32)
  if(VTK_VFW_SUPPORTS_CAPTURE)
    list(APPEND Module_SRCS vtkWin32VideoSource.cxx)
  endif()

  if(VTK_USE_MATROX_IMAGING)
    list(APPEND Module_SRCS vtkMILVideoSource.cxx)
    # Include directory needed for vtkMILVideoSource.cxx.
  endif()
endif()

vtk_module_library(vtkIOVideo ${Module_SRCS})
