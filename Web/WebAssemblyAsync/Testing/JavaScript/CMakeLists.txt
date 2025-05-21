set(vtk_nodejs_min_version "24.0.1")
find_package(NodeJS "${vtk_nodejs_min_version}" REQUIRED)
set(_vtk_testing_nodejs_exe "${NodeJS_INTERPRETER}")

if (CMAKE_HOST_WIN32)
  list(APPEND _vtk_node_args
    --experimental-wasm-jspi  # Remove this after https://gitlab.kitware.com/vtk/vtk/-/issues/19675 is closed
    --import "file://$<TARGET_FILE:VTK::WebAssemblyAsync>")
else ()
  list(APPEND _vtk_node_args
    --experimental-wasm-jspi  # Remove this after https://gitlab.kitware.com/vtk/vtk/-/issues/19675 is closed
    --import "$<TARGET_FILE:VTK::WebAssemblyAsync>")
endif ()
vtk_add_test_module_javascript_node(
  testFactories.mjs)
