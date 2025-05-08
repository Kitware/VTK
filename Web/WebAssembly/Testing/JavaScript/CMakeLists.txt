set(vtk_nodejs_min_version "23.8.0")
find_package(NodeJS "${vtk_nodejs_min_version}" REQUIRED)
if (VTK_WEBASSEMBLY_64_BIT)
  set(_vtk_node_args "--experimental-wasm-memory64")
endif ()
set(_vtk_testing_nodejs_exe "${NodeJS_INTERPRETER}")

if (CMAKE_HOST_WIN32)
  list(APPEND _vtk_node_args
    --import "file://$<TARGET_FILE:VTK::WasmSceneManager>")
else ()
  list(APPEND _vtk_node_args
    --import "$<TARGET_FILE:VTK::WasmSceneManager>")
endif ()
vtk_add_test_module_javascript_node(
  testBindRenderWindow.mjs
  testBlobs.mjs
  testInitialize.mjs,NO_DATA
  testInvoke.mjs
  testOSMesaRenderWindowPatch.mjs
  testSkipProperty.mjs
  testStates.mjs)
