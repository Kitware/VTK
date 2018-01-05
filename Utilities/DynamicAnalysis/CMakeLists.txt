CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/sanitizer_blacklist.txt.in
  ${CMAKE_CURRENT_BINARY_DIR}/sanitizer_blacklist.txt
  @ONLY
  )
