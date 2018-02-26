set(sources
  src/bitstream.c
  src/block1.h
  src/block2.h
  src/block3.h
  src/decode1d.c
  src/decode1f.c
  src/decode1i.c
  src/decode1l.c
  src/decode2d.c
  src/decode2f.c
  src/decode2i.c
  src/decode2l.c
  src/decode3d.c
  src/decode3f.c
  src/decode3i.c
  src/decode3l.c
  src/encode1d.c
  src/encode1f.c
  src/encode1i.c
  src/encode1l.c
  src/encode2d.c
  src/encode2f.c
  src/encode2i.c
  src/encode2l.c
  src/encode3d.c
  src/encode3f.c
  src/encode3i.c
  src/encode3l.c
  src/traitsd.h
  src/traitsf.h
  src/zfp.c)

vtk_add_library(vtkzfp ${sources})
target_include_directories(vtkzfp
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (NOT WIN32 OR CYGWIN)
  target_link_libraries(vtkzfp
    PRIVATE
      m)
endif ()
if (WIN32)
  target_compile_definitions(vtkzfp
    PRIVATE
      ZFP_SOURCE)
endif ()

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkzfp"
    COMPONENT Development)
endif()
