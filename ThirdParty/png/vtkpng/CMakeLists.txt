set(sources
  png.c
  pngerror.c
  pngget.c
  pngmem.c
  pngpread.c
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c)

set(headers
  png.h
  pngconf.h
  pnglibconf.h
  vtk_png_mangle.h)

vtk_module_add_module(VTK::png
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR vtkpng)
if (WIN32)
  set_target_properties(png
    PROPERTIES
      DEFINE_SYMBOL PNG_BUILD_DLL)
endif ()

# Link against the math library
if (UNIX)
  target_link_libraries(png
    PRIVATE
      m)
endif ()
