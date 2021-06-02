# Ensure that we're targeting 10.13.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_macos.cmake")
