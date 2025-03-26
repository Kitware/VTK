set(vtk_nodejs_min_version "22.0.0")
find_package(NodeJS "${vtk_nodejs_min_version}" REQUIRED)
if (VTK_WEBASSEMBLY_64_BIT)
  set(_vtk_node_args "--experimental-wasm-memory64")
endif ()
set(_vtk_testing_nodejs_exe "${NodeJS_INTERPRETER}")

list(APPEND _vtk_node_args
  --import "${PROJECT_BINARY_DIR}/bin/vtkWasmSceneManager.mjs")
vtk_add_test_module_javascript_node(
  testInitialize.mjs,NO_DATA
  testBlobs.mjs,
  testStates.mjs)
