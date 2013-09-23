set(Module_SRCS
  vtkGenericMovieWriter.cxx
  #vtkMPEG2Writer.cxx # Do we want to continue supporting?
  )

set_source_files_properties(
  vtkGenericMovieWriter
  ABSTRACT
  )

set (VTK_HAS_OGGTHEORA_SUPPORT)

if(vtkIOMovie_vtkoggtheora)
  set (VTK_HAS_OGGTHEORA_SUPPORT TRUE)
  list(APPEND Module_SRCS vtkOggTheoraWriter.cxx)
endif()

if(WIN32)
  # Check if VideoForWindows is supported. This also adds an option
  # VTK_USE_VIDEO_FOR_WINDOWS that is set to ON/OFF by default based on
  # whether VideoForWindows was found.
  include(vtkTestVideoForWindows)
endif()

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND Module_SRCS vtkAVIWriter.cxx)
endif()

# Configure the module specific settings into a module configured header.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkIOMovieConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h)

set(vtkIOMovie_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkIOMovieConfigure.h)

vtk_module_library(vtkIOMovie ${Module_SRCS})

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  target_link_libraries(vtkIOMovie LINK_PRIVATE vfw32)
endif()
