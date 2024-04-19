# SPDX generation do not work on renderless for some reasons
set(VTK_GENERATE_SPDX OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
