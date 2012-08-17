find_package(FFMPEG)
include(vtkTestFFMPEG)

set(Module_SRCS
  vtkFFMPEGWriter.cxx
  )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkFFMPEGConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkFFMPEGConfig.h @ONLY
  )

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vtkFFMPEGConfig.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
endif()

include_directories(${FFMPEG_INCLUDE_DIR})

set_source_files_properties( vtkFFMPEGWriter.cxx
  PROPERTIES COMPILE_DEFINITIONS __STDC_CONSTANT_MACROS
  )

set(_ffmpeg_libs ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY}
  ${FFMPEG_avutil_LIBRARY}
  )

if(NOT VTK_FFMPEG_HAS_IMG_CONVERT)
  list(APPEND _ffmpeg_libs ${FFMPEG_swscale_LIBRARY})
endif()

vtk_module_library(vtkIOFFMPEG ${Module_SRCS})

target_link_libraries(vtkIOFFMPEG ${_ffmpeg_libs})
