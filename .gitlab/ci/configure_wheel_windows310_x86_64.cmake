include("${CMAKE_CURRENT_LIST_DIR}/configure_wheel.cmake")

# Set some flags to avoid copious warnings.
#   - /wd4251: warnings about dll-interface of inherited classes
#   - /EHsc: set exception handler semantics
set(CMAKE_CXX_FLAGS "/wd4251 /EHsc" CACHE STRING "")
