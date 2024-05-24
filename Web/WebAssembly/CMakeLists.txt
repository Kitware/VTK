if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(FATAL_ERROR
    "The VTK::WebAssembly module requires Emscripten compiler.")
endif ()

set(classes
  vtkWasmSceneManager)

vtk_module_add_module(VTK::WebAssembly
  CLASSES ${classes})

vtk_add_test_mangling(VTK::WebAssembly)

set(_vtk_wasm_scene_manager_autoinit_mods)
get_property(_vtk_wasm_scene_manager_optional_deps GLOBAL
  PROPERTY "_vtk_module_VTK::WebAssembly_optional_depends")
foreach(_module IN LISTS _vtk_wasm_scene_manager_private_deps _vtk_wasm_scene_manager_optional_deps)
  if (NOT TARGET "${_module}")
    continue ()
  endif ()
  list(APPEND _vtk_wasm_scene_manager_autoinit_mods "${_module}")
endforeach()
vtk_module_autoinit(
  TARGETS WebAssembly
  MODULES ${_vtk_wasm_scene_manager_autoinit_mods})
# -----------------------------------------------------------------------------
# Emscripten compile+link options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
list(APPEND emscripten_link_options
  "-lembind"
  "--extern-post-js=${CMAKE_CURRENT_SOURCE_DIR}/post.js"
  # "--embind-emit-tsd=vtkWasmSceneManager.ts"
  #"--memoryprofiler"
  #"--cpuprofiler"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sALLOW_TABLE_GROWTH=1"
  "-sEXPORT_NAME=vtkWasmSceneManager"
  "-sENVIRONMENT=node,web"
  "-sEXPORTED_RUNTIME_METHODS=['addFunction','UTF8ToString']"
  # "-sEXCEPTION_DEBUG=1" # prints stack trace for uncaught C++ exceptions from VTK (very rare, but PITA to figure out)
  # "-sGL_DEBUG=1"
  # "-sGL_ASSERTIONS=1"
  # "-sTRACE_WEBGL_CALLS=1"
  )
if (CMAKE_SIZEOF_VOID_P EQUAL "8")
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=16GB")
else ()
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=4GB")
endif ()
# -----------------------------------------------------------------------------
# Optimizations
# -----------------------------------------------------------------------------
set(emscripten_optimizations)
set(emscripten_debug_options)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(vtk_scene_manager_wasm_optimize "BEST")
  set(vtk_scene_manager_wasm_debuginfo "NONE")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(vtk_scene_manager_wasm_optimize "SMALLEST_WITH_CLOSURE")
  set(vtk_scene_manager_wasm_debuginfo "NONE")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(vtk_scene_manager_wasm_optimize "MORE")
  set(vtk_scene_manager_wasm_debuginfo "PROFILE")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(vtk_scene_manager_wasm_optimize "NO_OPTIMIZATION")
  set(vtk_scene_manager_wasm_debuginfo "DEBUG_NATIVE")
endif ()
set(vtk_scene_manager_wasm_optimize_NO_OPTIMIZATION "-O0")
set(vtk_scene_manager_wasm_optimize_LITTLE "-O1")
set(vtk_scene_manager_wasm_optimize_MORE "-O2")
set(vtk_scene_manager_wasm_optimize_BEST "-O3")
set(vtk_scene_manager_wasm_optimize_SMALLEST "-Os")
set(vtk_scene_manager_wasm_optimize_SMALLEST_WITH_CLOSURE "-Oz")
set(vtk_scene_manager_wasm_optimize_SMALLEST_WITH_CLOSURE_link "--closure=1")

if (DEFINED "vtk_scene_manager_wasm_optimize_${vtk_scene_manager_wasm_optimize}")
  list(APPEND emscripten_optimizations
    ${vtk_scene_manager_wasm_optimize_${vtk_scene_manager_wasm_optimize}})
  list(APPEND emscripten_link_options
    ${vtk_scene_manager_wasm_optimize_${vtk_scene_manager_wasm_optimize}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for vtk_scene_manager_wasm_optimize=${vtk_scene_manager_wasm_optimize}")
endif ()

set(vtk_scene_manager_wasm_debuginfo_NONE "-g0")
set(vtk_scene_manager_wasm_debuginfo_READABLE_JS "-g1")
set(vtk_scene_manager_wasm_debuginfo_PROFILE "-g2")
set(vtk_scene_manager_wasm_debuginfo_DEBUG_NATIVE "-g3")
set(vtk_scene_manager_wasm_debuginfo_DEBUG_NATIVE_link "-sASSERTIONS=1")
if (DEFINED "vtk_scene_manager_wasm_debuginfo_${vtk_scene_manager_wasm_debuginfo}")
  list(APPEND emscripten_debug_options
    ${vtk_scene_manager_wasm_debuginfo_${vtk_scene_manager_wasm_debuginfo}})
  list(APPEND emscripten_link_options
    ${vtk_scene_manager_wasm_debuginfo_${vtk_scene_manager_wasm_debuginfo}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for vtk_scene_manager_wasm_debuginfo=${vtk_scene_manager_wasm_debuginfo}")
endif ()

vtk_module_add_executable(WasmSceneManager
  BASENAME vtkWasmSceneManager
  vtkWasmSceneManagerEmBinding.cxx)
add_executable("VTK::WasmSceneManager" ALIAS
  WasmSceneManager)
target_link_libraries(WasmSceneManager
  PRIVATE
    VTK::WebAssembly)
target_compile_options(WasmSceneManager
  PRIVATE
    ${emscripten_compile_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options})
target_link_options(WasmSceneManager
  PRIVATE
    ${emscripten_link_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options})
set_target_properties(WasmSceneManager
  PROPERTIES
  OUTPUT_NAME "vtkWasmSceneManager"
  SUFFIX ".mjs")
# [cmake/cmake#20745](https://gitlab.kitware.com/cmake/cmake/-/issues/20745)
# CMake doesn't install multiple files associated with an executable target.
install(FILES
  "$<TARGET_FILE_DIR:WasmSceneManager>/vtkWasmSceneManager.wasm"
  DESTINATION ${CMAKE_INSTALL_BINDIR})
