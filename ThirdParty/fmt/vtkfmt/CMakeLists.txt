set (sources
  src/format.cc
  src/os.cc)

set (headers
  vtkfmt/args.h
  vtkfmt/chrono.h
  vtkfmt/color.h
  vtkfmt/compile.h
  vtkfmt/core.h
  vtkfmt/format.h
  vtkfmt/format-inl.h
  vtkfmt/os.h
  vtkfmt/ostream.h
  vtkfmt/printf.h
  vtkfmt/ranges.h
  vtkfmt/std.h
  vtkfmt/xchar.h)

#-----------------------------------------
include(support/cmake/cxx14.cmake)

vtk_module_add_module(VTK::fmt
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkfmt")

vtk_module_compile_features(VTK::fmt
  INTERFACE cxx_variadic_templates)

include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(_strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
else ()
  check_symbol_exists(strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)
endif ()

if (HAVE_STRTOD_L)
  vtk_module_definitions(VTK::fmt PUBLIC FMT_LOCALE)
endif ()

if (BUILD_SHARED_LIBS)
  vtk_module_definitions(VTK::fmt PRIVATE FMT_EXPORT INTERFACE FMT_SHARED)
endif ()
