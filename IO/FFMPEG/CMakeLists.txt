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

set(_ffmpeg_libs ${FFMPEG_LIBAVFORMAT_LIBRARIES} ${FFMPEG_LIBAVCODEC_LIBRARIES}
  ${FFMPEG_LIBAVUTIL_LIBRARIES}
  )

if(NOT VTK_FFMPEG_HAS_IMG_CONVERT)
  list(APPEND _ffmpeg_libs ${FFMPEG_LIBSWSCALE_LIBRARIES})
endif()

vtk_module_library(vtkIOFFMPEG ${Module_SRCS})

vtk_module_link_libraries(vtkIOFFMPEG LINK_PRIVATE ${_ffmpeg_libs})
