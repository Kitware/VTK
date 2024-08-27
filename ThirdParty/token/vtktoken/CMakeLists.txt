if (FALSE) # XXX(kitware): Let VTK dictate version constraints.
cmake_minimum_required(VERSION 3.12)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  set(CMAKE_CXX_EXTENSIONS FALSE)
elseif(CMAKE_CXX_STANDARD LESS 11)
  set(CMAKE_CXX_STANDARD 11)
endif()

include(cmake/tokenVersion.cmake)
project(token VERSION ${token_VERSION} LANGUAGES CXX)

set(token_cmake_dir ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${token_cmake_dir})


include(tokenMacros)
include(tokenTestingMacros)
include(tokenSetup)
include(tokenOptions)
include(tokenPreInstall)

add_subdirectory(token)

# XXX(kitware): install via VTK's module system
# include(tokenPostInstall)
