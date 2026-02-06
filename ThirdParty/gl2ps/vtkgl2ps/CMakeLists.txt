set(sources
  gl2ps.c)

set(headers
  gl2ps.h
  vtk_gl2ps_mangle.h)

vtk_module_add_module(VTK::gl2ps
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkgl2ps")
vtk_module_set_property(VTK::gl2ps
  PROPERTY  DEFINE_SYMBOL
  VALUE     GL2PSDLL_EXPORTS)

if (BUILD_SHARED_LIBS)
  vtk_module_definitions(VTK::gl2ps
    PRIVATE
      GL2PSDLL)
endif ()

vtk_module_definitions(VTK::gl2ps
  PRIVATE
    HAVE_ZLIB
    HAVE_PNG)

if (UNIX)
  vtk_module_link(VTK::gl2ps
    PRIVATE
      m)
endif ()
