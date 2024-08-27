# Include GNU install directory module to detect where to install
# files on Linux/Unix systems (e.g., lib vs lib64)
include(GNUInstallDirs)

# Include export-header functions so we can expose symbols in dynamic libraries.
include(GenerateExportHeader)

# Force LIBRARY_OUTPUT_PATH to be a cache variable, whether it was already defined or not.
if (NOT DEFINED LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR} CACHE INTERNAL "Directory for all libraries.")
else()
  set(LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH} CACHE INTERNAL "Directory for all libraries.")
endif()
# Set the directory where the binaries will be stored
set(EXECUTABLE_OUTPUT_PATH         ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# Set the directory where cmake configuration files are installed. The
# convention for this directory's location is OS-dependent. See
# https://cmake.org/cmake/help/latest/command/find_package.html#search-procedure
# for more information.
set(token_INSTALL_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${token_VERSION})

if (FALSE) # XXX(kitware): Do not pollute VTK's cache with token options.
# Specify where the packaging files will be installed
set(token_PACKAGE_INSTALL_DIR
  "${CMAKE_INSTALL_LIBDIR}/cmake"
  CACHE STRING "The directory relative to CMAKE_PREFIX_PATH where tokenConfig.cmake is installed"
)
mark_as_advanced(token_PACKAGE_INSTALL_DIR)
endif()
