set(vtk_nodejs_min_version "18.8.0")
find_package(NodeJS "${vtk_nodejs_min_version}" REQUIRED)
set(_vtk_testing_nodejs_exe "${NodeJS_INTERPRETER}")

set(_vtk_node_args
  --import "${PROJECT_BINARY_DIR}/bin/vtkWasmSceneManager-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.mjs")
vtk_add_test_module_javascript_node(
  testInitialize.mjs,NO_DATA
  testBlobs.mjs,
  testStates.mjs)
