set(Module_SRCS
  vtkGenericMovieWriter.cxx
  #vtkMPEG2Writer.cxx # Do we want to continue supporting?
  )

set_source_files_properties(
  vtkGenericMovieWriter
  ABSTRACT
  )

if(vtkIOMovie_vtkoggtheora)
  list(APPEND Module_SRCS vtkOggTheoraWriter.cxx)
endif()

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  include(vtkTestVideoForWindows)
  list(APPEND Module_SRCS vtkAVIWriter.cxx)
endif()

vtk_module_library(vtkIOMovie ${Module_SRCS})

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  target_link_libraries(vtkIOMovie vfw32)
endif()
