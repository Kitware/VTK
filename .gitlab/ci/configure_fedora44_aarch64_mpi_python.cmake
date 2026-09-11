# Disable modules for which there are missing dependencies.
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns

# Enable Catalyst and Conduit to test their external integration
set(VTK_ENABLE_CATALYST ON CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_conduit YES CACHE STRING "")
set(VTK_MODULE_ENABLE_VTK_fides YES CACHE STRING "")

set(CMAKE_PREFIX_PATH "/opt/catalyst-ext/mpich;/opt/conduit/mpich" CACHE STRING "")
# Conduit has a `cmake_minimum_required(3.8)` call which warns with new CMake.
# Add a phantom `...3.13` to suppress the warning.
# https://github.com/llnl/conduit/issues/1697
set(CMAKE_POLICY_VERSION_MINIMUM "3.13" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora44.cmake")
