cmake_minimum_required(VERSION 3.14...4.0)

project(
  vtk-wheel-sdk
  VERSION ${SKBUILD_PROJECT_VERSION}
  DESCRIPTION "VTK SDK python distributions"
  HOMEPAGE_URL "https://vtk.org"
  LANGUAGES NONE
)

find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED
)

# Write a simple _version.py file to install later
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/_version.py "
__version__ = version = '${SKBUILD_PROJECT_VERSION_FULL}'
")

# ----------------------------------------------------------------------------
# Install content of the vtk-wheel-sdk archive

set(VTK_SDK_INSTALL_DIR "content") # used by configure_file too

# Append "/" after ${extract_dir} to ensure folder content is copied to the
# destination, instead of the folder itself.
install(
  DIRECTORY ${VTK_INSTALL_DIR}/
  DESTINATION vtk_sdk/${VTK_SDK_INSTALL_DIR}
  PATTERN "bin/*" EXCLUDE
)

install(
  DIRECTORY ${VTK_INSTALL_DIR}/bin/
  DESTINATION vtk_sdk/${VTK_SDK_INSTALL_DIR}/bin
  PATTERN
    "*"
    PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
)

# Create and install "cmake.prefix" entry point files
# These files needs to be in a location that doesn't contain any '.',
# but the folder of VTK real config module must be located in a ".data" folder.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vtk-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtk_sdk/cmake/vtk-config.cmake
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vtk-config-version.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtk_sdk/cmake/vtk-config-version.cmake
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/_version.py
  DESTINATION vtk_sdk
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/vtk_sdk/cmake/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/vtk_sdk/cmake/vtk-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/vtk_sdk/cmake/vtk-config-version.cmake
  DESTINATION vtk_sdk/cmake
)
