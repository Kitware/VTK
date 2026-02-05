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
  vtk_module_set_property(VTK::png
    PROPERTY  DEFINE_SYMBOL
    VALUE     PNG_BUILD_DLL)
endif ()

# Link against the math library
if (UNIX)
  vtk_module_link(VTK::png
    PRIVATE
      m)
endif ()
