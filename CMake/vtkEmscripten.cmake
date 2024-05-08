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

set (default_wasm_threads OFF)
include(vtkTesting)
if (VTK_BUILD_TESTING)
  # Tests want to use synchronous XHR in order to access data and image files from the host filesystem outside of the wasm sandbox.
  # Since synchronous XHR is deprecated outside of a web worker, VTK cannot have a test wait for data loading on the main browser thread.
  # By enabling pthreads support, emscripten will be asked to run `main(argc, argv)` on a web worker and proxy function calls relating to the
  # DOM, FS API over to the main UI thread.
  # https://emscripten.org/docs/porting/pthreads.html#additional-flags
  set (default_wasm_threads ON)
endif ()
option(VTK_WEBASSEMBLY_THREADS "Enable threading support in wasm. Adds -pthread compile and link flags." ${default_wasm_threads})
