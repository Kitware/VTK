# Emscripten.cmake automatically enables 64-bit when the shell defines CMAKE_C_FLAGS="-sMEMORY64=1".
# This exposes a proper CMake option to switch b/w 32 and 64 bit.
option(VTK_WEBASSEMBLY_64_BIT "Enable support for 64-bit memory in wasm. Adds -sMEMORY64=1 compile and link flags." OFF)
if (VTK_WEBASSEMBLY_64_BIT)
  # stick to what Emscripten.cmake does.
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
if (VTK_WEBASSEMBLY_THREADS)
  set(VTK_WEBASSEMBLY_THREAD_POOL_SIZE 0 CACHE STRING "Limit the maximum number of threads in the WASM backend (default:0, unlimited).")
endif ()
if (NOT VTK_WEBASSEMBLY_THREAD_POOL_SIZE MATCHES "^[0-9]+$")
  message(FATAL_ERROR "VTK_WEBASSEMBLY_THREAD_POOL_SIZE must be a non-negative integer")
endif()

include(ProcessorCount)
ProcessorCount(nproc)
set(VTK_WEBASSEMBLY_JOB_POOL_LINK_SIZE ${nproc} CACHE STRING "Size of the job pool for linking wasm targets. Adjust as needed to avoid OOM errors")
if(CMAKE_GENERATOR MATCHES "Ninja")
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS "wasm_link_job_pool=${VTK_WEBASSEMBLY_JOB_POOL_LINK_SIZE}")
  set(CMAKE_JOB_POOL_LINK wasm_link_job_pool)
else ()
  message(WARNING "Some targets may not link successfully! Job pooling is only available with Ninja generators.")
endif()
