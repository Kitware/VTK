# Add rpath entries for Xcode frameworks.
set(CMAKE_BUILD_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_macos.cmake")
