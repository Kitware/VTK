#
# Build the documentation
#
INCLUDE (${CMAKE_ROOT}/Modules/DocumentationVTK.cmake OPTIONAL)

IF (BUILD_DOCUMENTATION)

  #
  # Configure the script and the doxyfile, then add target
  #
  CONFIGURE_FILE(
    ${VTK_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
    ${VTK_BINARY_DIR}/Utilities/Doxygen/doxyfile)

  CONFIGURE_FILE(
    ${VTK_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
    ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

  ADD_CUSTOM_TARGET(DoxygenDoc
    ${BASH}
    ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

ENDIF (BUILD_DOCUMENTATION)
