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

vtk_add_library(vtklz4 ${sources} ${headers})
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtklz4/lib"
    COMPONENT Development)
endif()

target_compile_definitions(vtklz4
  PRIVATE
    "LZ4_VERSION=\"1.8.0\"")
if (WIN32)
  target_compile_definitions(vtklz4
    PRIVATE
      LZ4_DLL_EXPORT=1
    INTERFACE
      LZ4_DLL_IMPORT=1)
endif ()
target_include_directories(vtklz4
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib")
