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

if(WIN32)
  # Should we force this on Windows?
  list(APPEND Module_SRCS vtkAVIWriter.cxx)
endif()

vtk_module_library(vtkIOMovie ${Module_SRCS})

if(WIN32)
  target_link_libraries(vtkIOMovie vfw32)
endif()
