vtk_module_test_data(
  Data/WasmSceneManager/scalar-bar-widget.blobs.json
  Data/WasmSceneManager/scalar-bar-widget.states.json
  Data/WasmSceneManager/simple.blobs.json)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  add_subdirectory(JavaScript)
endif ()
