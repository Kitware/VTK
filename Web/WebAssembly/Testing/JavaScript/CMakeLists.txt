set(vtk_nodejs_min_version "24.0.1")
find_package(NodeJS "${vtk_nodejs_min_version}" REQUIRED)
set(_vtk_testing_nodejs_exe "${NodeJS_INTERPRETER}")

if (CMAKE_HOST_WIN32)
  list(APPEND _vtk_node_args
    --import "file://$<TARGET_FILE:VTK::WebAssembly>")
else ()
  list(APPEND _vtk_node_args
    --import "$<TARGET_FILE:VTK::WebAssembly>")
endif ()
vtk_add_test_module_javascript_node(
  testBindRenderWindow.mjs
  testBlobs.mjs
  testInitialize.mjs,NO_DATA
  testInvoke.mjs
  testNull.mjs
  testOSMesaRenderWindowPatch.mjs
  testSkipProperty.mjs
  testStates.mjs)
