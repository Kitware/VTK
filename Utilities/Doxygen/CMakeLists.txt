#
# Build the documentation
#
INCLUDE (${CMAKE_ROOT}/Modules/Documentation.cmake OPTIONAL)

IF (BUILD_DOCUMENTATION)
  OPTION(DOXYGEN_SHORT_NAMES "Build Doxygen of VTK using short file names" OFF)
  MARK_AS_ADVANCED(DOXYGEN_SHORT_NAMES)
  IF(DOXYGEN_SHORT_NAMES)
    SET(DOXYGEN_SHORT_NAMES YES)
  ELSE(DOXYGEN_SHORT_NAMES)
    SET(DOXYGEN_SHORT_NAMES NO)
  ENDIF(DOXYGEN_SHORT_NAMES)

  #
  # Configure the script and the doxyfile, then add target
  #
  IF(NOT DOT_PATH)
    GET_FILENAME_COMPONENT(DOT_PATH ${DOT} PATH)
  ENDIF(NOT DOT_PATH)

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

INCLUDE (${VTK_SOURCE_DIR}/Utilities/Doxygen/LocalUserOptions.cmake OPTIONAL)

INSTALL_FILES(/lib/vtk/doxygen "\\.css$")
INSTALL_FILES(/lib/vtk/doxygen "\\.gif$")
INSTALL_FILES(/lib/vtk/doxygen "\\.html$")
INSTALL_FILES(/lib/vtk/doxygen "\\.pl$")
INSTALL_FILES(/lib/vtk/doxygen "\\.stop$")
INSTALL_FILES(/lib/vtk/doxygen .txt authors doc_readme)
