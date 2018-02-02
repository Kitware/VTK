set(sources
  gl2ps.c)

set(headers
  gl2ps.h
  vtk_gl2ps_mangle.h)

vtk_add_library(vtkgl2ps ${sources} ${headers})
target_link_libraries(vtkgl2ps PUBLIC ${vtkpng_LIBRARIES} ${vtkzlib_LIBRARIES})
include(vtkOpenGL)
vtk_opengl_link(vtkgl2ps)

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkgl2ps"
    COMPONENT Development)
endif()
set_target_properties(vtkgl2ps
  PROPERTIES
    DEFINE_SYMBOL GL2PSDLL_EXPORTS)

if (BUILD_SHARED_LIBS)
  target_compile_definitions(vtkgl2ps
    PRIVATE
      GL2PSDLL)
endif ()

target_compile_definitions(vtkgl2ps
  PRIVATE
    HAVE_ZLIB
    HAVE_PNG)

if (UNIX)
  target_link_libraries(vtkgl2ps
    PRIVATE
      m)
endif ()
