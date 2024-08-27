# Install a package targets file
#
# Setup the exports for the library when used from an installed location
install(
  EXPORT ${PROJECT_NAME}
  DESTINATION ${token_INSTALL_CONFIG_DIR}
  FILE ${PROJECT_NAME}Targets.cmake
)

export(EXPORT ${PROJECT_NAME} FILE "${PROJECT_BINARY_DIR}/${token_INSTALL_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")

set(token_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")
set(token_cmake_build_dir "${CMAKE_BINARY_DIR}/${token_INSTALL_CONFIG_DIR}")
set(token_cmake_destination "${token_INSTALL_CONFIG_DIR}")
include(tokenInstallCMakePackage)

set(token_MODULE_DIR "${PROJECT_SOURCE_DIR}/cmake")
set(token_CONFIG_DIR "${PROJECT_BINARY_DIR}")
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  ${token_INSTALL_CONFIG_DIR}/${PROJECT_NAME}Config.cmake
  @ONLY)

# Create an install package configuration file
#
# Setup the config file for exports that stores what other thirdparty
# packages we need to search for ( MOAB, Remus, etc ) for when using the
# install version of token

# If we are performing a relocatable install, we must erase the hard-coded
# install paths we set in token_prefix_path before constructing the install
# package configuration file
if (token_RELOCATABLE_INSTALL)
  set(token_prefix_path)
endif()

set(token_MODULE_DIR "\${CMAKE_CURRENT_LIST_DIR}")
set(token_CONFIG_DIR "\${CMAKE_CURRENT_LIST_DIR}")
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
  @ONLY)

install (FILES ${PROJECT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
         DESTINATION ${token_INSTALL_CONFIG_DIR})
