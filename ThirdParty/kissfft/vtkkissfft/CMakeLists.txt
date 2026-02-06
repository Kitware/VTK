if (NOT DEFINED KISSFFT_SCALAR_TYPE)
  set(KISSFFT_SCALAR_TYPE "double")
endif ()

set(sources
  kiss_fft.c
  tools/kiss_fastfir.c
  tools/kiss_fftnd.c
  tools/kiss_fftndr.c
  tools/kiss_fftr.c)

set(headers
  kiss_fft_exports.h
  _kiss_fft_guts.h
  kiss_fft.h
  tools/kfc.h
  tools/kiss_fftnd.h
  tools/kiss_fftndr.h
  tools/kiss_fftr.h
  tools/vtkkissfft_fftnd.h
  tools/vtkkissfft_fftndr.h
  tools/vtkkissfft_fftr.h
  vtkkissfft_fft.h
  vtk_kiss_fft_mangle.h)
vtk_module_add_module(VTK::kissfft
  SOURCES ${sources}
  HEADERS ${headers}
  HEADER_DIRECTORIES
  HEADERS_SUBDIR "vtkkissfft")

vtk_module_definitions(VTK::kissfft
  PUBLIC "kiss_fft_scalar=${KISSFFT_SCALAR_TYPE}")
if (WIN32 AND BUILD_SHARED_LIBS)
  vtk_module_definitions(VTK::kissfft
    PRIVATE
      KISSFFT_DLL_EXPORT=1
    INTERFACE
      KISSFFT_DLL_IMPORT=1)
endif ()
vtk_module_include(VTK::kissfft
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

if (UNIX)
  vtk_module_link(VTK::kissfft PRIVATE m)
endif ()
