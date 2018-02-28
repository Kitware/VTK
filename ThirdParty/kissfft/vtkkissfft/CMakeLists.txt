project(kissfft)

set(KISSFFT_SCALAR_TYPE "double" CACHE STRING "Scalar data type")
set_property(CACHE KISSFFT_SCALAR_TYPE PROPERTY STRINGS "float" "double")
mark_as_advanced(KISSFFT_SCALAR_TYPE)

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
  tools/vtk_kiss_fftnd.h
  tools/vtk_kiss_fftndr.h
  tools/vtk_kiss_fftr.h
  vtk_kiss_fft.h
  vtk_kiss_fft_mangle.h
  )
vtk_add_library(vtkkissfft ${sources} ${headers})

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkkissfft"
    COMPONENT Development)
endif()

target_compile_definitions(vtkkissfft
  PUBLIC "kiss_fft_scalar=${KISSFFT_SCALAR_TYPE}")
if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(vtkkissfft
    PRIVATE
      KISSFFT_DLL_EXPORT=1
    INTERFACE
      KISSFFT_DLL_IMPORT=1)
endif ()
target_include_directories(vtkkissfft PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
