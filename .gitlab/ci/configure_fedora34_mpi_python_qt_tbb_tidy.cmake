set(CMAKE_C_CLANG_TIDY "clang-tidy-cache" CACHE FILEPATH "")
set(CMAKE_CXX_CLANG_TIDY "clang-tidy-cache" CACHE FILEPATH "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
