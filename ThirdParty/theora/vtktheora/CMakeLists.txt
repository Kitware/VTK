set(sources
  # encoder
  lib/analyze.c
  lib/encapiwrapper.c
  lib/encfrag.c
  lib/encinfo.c
  lib/encode.c
  lib/enquant.c
  lib/fdct.c
  lib/huffenc.c
  lib/mathops.c
  lib/mcenc.c
  lib/rate.c
  lib/tokenize.c

  # decoder
  lib/bitpack.c
  lib/decapiwrapper.c
  lib/decinfo.c
  lib/decode.c
  lib/dequant.c
  lib/huffdec.c
  lib/info.c

  # shared
  lib/apiwrapper.c
  lib/fragment.c
  lib/idct.c
  lib/internal.c
  lib/quant.c
  lib/state.c)

set(headers
  include/theora/codec.h
  include/theora/theora.h
  include/theora/theoradec.h
  include/theora/theoraenc.h
  include/theora/vtk_theora_mangle.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtktheora_export.h")

vtk_module_add_module(VTK::theora
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtktheora/include/theora")
target_include_directories(theora
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
set_target_properties(theora
  PROPERTIES
    C_VISIBILITY_PRESET "hidden")

include(GenerateExportHeader)
generate_export_header(theora
  EXPORT_MACRO_NAME VTKTHEORA_EXPORT
  EXPORT_FILE_NAME  vtktheora_export.h)

include(CheckCCompilerFlag)
function (vtktheora_suppress_warning warning)
  check_c_compiler_flag("-Wno-${warning}" "have_flag_${warning}")
  if (have_flag_${warning})
    target_compile_options(theora
      PRIVATE
        "-Wno-${warning}")
  endif ()
endfunction ()
# Suppress these warnings. libtheora has an odd style and the library is tested
# well enough that we can ignore them.
vtktheora_suppress_warning("logical-op-parentheses")
vtktheora_suppress_warning("parentheses")
vtktheora_suppress_warning("shift-op-parentheses")
vtktheora_suppress_warning("tautological-compare")
