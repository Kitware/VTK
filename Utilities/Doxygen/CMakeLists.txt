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

IF(NOT VTK_INSTALL_NO_DOCUMENTATION)
  MACRO(__vtk_install_documentation_files glob)
    FILE(GLOB __files "${CMAKE_CURRENT_SOURCE_DIR}/${glob}")
    INSTALL(FILES ${__files}
      DESTINATION ${VTK_INSTALL_DOXYGEN_DIR_CM24}
      COMPONENT Development)
  ENDMACRO(__vtk_install_documentation_files)
  __vtk_install_documentation_files("*.css")
  __vtk_install_documentation_files("*.gif")
  __vtk_install_documentation_files("*.html")
  __vtk_install_documentation_files("*.pl")
  __vtk_install_documentation_files("*.stop")
  INSTALL(FILES authors.txt doc_readme.txt
    DESTINATION ${VTK_INSTALL_DOXYGEN_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DOCUMENTATION)
