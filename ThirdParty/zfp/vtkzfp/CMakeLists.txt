include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/inc"
  "${CMAKE_CURRENT_BINARY_DIR}")

set(zfp_sources
  src/bitstream.c
  src/decode1d.c
  src/decode1f.c
  src/decode2d.c
  src/decode2f.c
  src/decode3d.c
  src/decode3f.c
  src/encode1d.c
  src/encode1f.c
  src/encode2d.c
  src/encode2f.c
  src/encode3d.c
  src/encode3f.c
  src/zfp.c)
vtk_add_library(vtkzfp ${zfp_sources})
include(GenerateExportHeader)
generate_export_header(vtkzfp
  EXPORT_MACRO_NAME VTKZFP_EXPORT)
if (NOT WIN32 OR CYGWIN)
  target_link_libraries(vtkzfp m)
endif ()

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkzfp"
    COMPONENT   Development)
endif ()
