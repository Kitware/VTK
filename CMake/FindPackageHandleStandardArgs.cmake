
# include the the latest version of FindPackageHandleStandardArgs.
if(CMAKE_VERSION VERSION_LESS 3.6.0)
  include(${CMAKE_CURRENT_LIST_DIR}/NewCMake/FindPackageHandleStandardArgs.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
endif()
