# Emscripten.cmake automatically enables 64-bit when the shell defines CMAKE_C_FLAGS="-sMEMORY64=1".
# This exposes a proper CMake option to switch b/w 32 and 64 bit.
option(VTK_WEBASSEMBLY_64_BIT "Enable support for 64-bit memory in wasm. Adds -sMEMORY64=1 compile and link flags." OFF)
if (VTK_WEBASSEMBLY_64_BIT)
  # stick to what Emscripten.cmake does.
  set(CMAKE_CROSSCOMPILING_EMULATOR "${CMAKE_CROSSCOMPILING_EMULATOR}" "--experimental-wasm-memory64")
  set(CMAKE_LIBRARY_ARCHITECTURE "wasm64-emscripten")
  set(CMAKE_SIZEOF_VOID_P 8)
  set(CMAKE_C_SIZEOF_DATA_PTR 8)
  set(CMAKE_CXX_SIZEOF_DATA_PTR 8)
else ()
  # stick to what Emscripten.cmake does.
  set(CMAKE_LIBRARY_ARCHITECTURE "wasm32-emscripten")
  set(CMAKE_SIZEOF_VOID_P 4)
  set(CMAKE_C_SIZEOF_DATA_PTR 4)
  set(CMAKE_CXX_SIZEOF_DATA_PTR 4)
endif ()

option(VTK_WEBASSEMBLY_THREADS "Enable threading support in wasm. Adds -pthread compile and link flags." OFF)
option(VTK_WEBASSEMBLY_EXCEPTIONS "Enable exception support in wasm. Adds -fexceptions compile and link flags." OFF)

# wasm linking is already multithreaded. Here, we ensure targets are linked one at a time to avoid
# OOM errors and file lock contention.
if(CMAKE_GENERATOR MATCHES "Ninja")
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS wasm64_link_job_pool=1)
  set(CMAKE_JOB_POOL_LINK wasm64_link_job_pool)
else ()
  message(WARNING "Some targets may not link successfully! Job pooling is only available with Ninja generators.")
endif()
