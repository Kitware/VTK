SUBDIRS(internals lib resources)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/.NoDartCoverage
  ${CMAKE_CURRENT_BINARY_DIR}/.NoDartCoverage
  COPY_ONLY)
