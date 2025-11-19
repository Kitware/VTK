# Disable modules for which there are missing dependencies.
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora42.cmake")
