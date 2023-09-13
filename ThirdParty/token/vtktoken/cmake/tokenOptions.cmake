include(CMakeDependentOption)
include(CheckCXXSymbolExists)
include(CheckIncludeFileCXX)

# XXX(kitware): No need for this within VTK
# find_package(nlohmann_json REQUIRED)

# XXX(kitware): Do not build third-party tests in VTK.
option(token_ENABLE_TESTING "Build tests?" OFF)
mark_as_advanced(token_ENABLE_TESTING)
if (token_ENABLE_TESTING)
  set(token_PUBLIC_DROP_SITE ON CACHE BOOL "Submit test results to public dashboards.")
  mark_as_advanced(token_PUBLIC_DROP_SITE)

  enable_testing()
  include(CTest)

  # Do not report some warnings from generated code to the dashboard:
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CTestCustom.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake")

endif()

# On some *nix distributions, RPath has been deprecated in favor for RunPath. One
# difference between these concepts is that RunPath is not inherited. This causes
# issues with locating libraries without manually setting LD_LIBRARY_PATH when the
# indirect dependency is in a nonstandard location. This flag switches the compiler
# to use RPath in favor of RunPath.
cmake_dependent_option(token_FORCE_OLD_DTAGS "Use deprecated RPath in favor of RunPath"
  OFF "UNIX;NOT APPLE" OFF)
mark_as_advanced(token_FORCE_OLD_DTAGS)
if (token_FORCE_OLD_DTAGS)
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--disable-new-dtags")
endif()

# XXX(kitware): Do not build third-party documentation in VTK or expose a custom namespace.
if (FALSE)
# token_BUILD_DOCUMENTATION is an enumerated option:
# never  == No documentation, and no documentation tools are required.
# manual == Only build when requested; documentation tools (doxygen and
#           sphinx) must be located during configuration.
# always == Build documentation as part of the default target; documentation
#           tools are required. This is useful for automated builds that
#           need "make; make install" to work, since installation will fail
#           if no documentation is built.
set(token_BUILD_DOCUMENTATION
  "never" CACHE STRING "When to build Doxygen- (and eventually Sphinx-) generated documentation.")
set_property(CACHE token_BUILD_DOCUMENTATION PROPERTY STRINGS never manual always)
if (NOT token_BUILD_DOCUMENTATION STREQUAL "never")
  find_package(Doxygen)
  # find_package(Sphinx)
endif()

# token_NAMESPACE is the name of a namespace to use for this library's API.
# By default, it will be set to "token" but it can be provided externally
# in order for multiple versions of the library to be linked into a single
# executable.
if (NOT DEFINED CACHE{token_NAMESPACE})
  set(token_NAMESPACE "" CACHE STRING "A C++ namespace for this library's API.")
  mark_as_advanced(token_NAMESPACE)
endif()
endif() # XXX
# Do not allow an empty (anonymous) namespace for our public API:
set(token_CXX_NAMESPACE "${token_NAMESPACE}")
if ("${token_CXX_NAMESPACE}" STREQUAL "")
  set(token_CXX_NAMESPACE "token")
endif()
