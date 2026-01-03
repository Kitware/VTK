if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(FATAL_ERROR
    "The VTK::WebAssemblyAsync module requires Emscripten compiler.")
endif ()

vtk_module_add_serdes_wasm_package(
  ASYNC
  MODULE VTK::WebAssemblyAsync
  OUTPUT_NAME vtkWebAssemblyAsync
  # Uncomment the next line when quick linking is desired for faster dev cycles.
  # EXTRA_LINK_OPTIONS "-O1"
)
