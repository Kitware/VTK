if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(FATAL_ERROR
    "The VTK::WebAssembly module requires Emscripten compiler.")
endif ()

vtk_module_add_serdes_wasm_package(
  MODULE VTK::WebAssembly
  OUTPUT_NAME vtkWebAssembly
)
