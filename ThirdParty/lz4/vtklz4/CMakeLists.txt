include(GenerateExportHeader)

set(sources
  lib/lz4.c
  lib/lz4hc.c
  lib/lz4frame.c
  lib/xxhash.c)

set(headers
  lib/lz4.h
  lib/lz4hc.h
  lib/lz4frame.h
  lib/lz4frame_static.h)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_C_STANDARD 99)

vtk_module_add_module(VTK::lz4
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtklz4/lib")
target_compile_definitions(lz4
  PRIVATE
    "LZ4_VERSION=\"1.8.0\"")
if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(lz4
    PRIVATE
      LZ4_DLL_EXPORT=1
    INTERFACE
      LZ4_DLL_IMPORT=1)
endif ()
target_include_directories(lz4
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib")
