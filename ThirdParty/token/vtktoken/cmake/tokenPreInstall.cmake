# Save compile-time options for use by other packages
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/Options.h.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}/Options.h
  @ONLY)

# This file is installed by adding it as a source file
# in `../token/CMakeLists.txt`.
# install (FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}/Options.h
#   DESTINATION include/${PROJECT_NAME}/${token_VERSION}/token)
