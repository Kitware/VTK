#
# Build the documentation
#

INCLUDE (${CMAKE_ROOT}/Modules/Documentation.cmake OPTIONAL)

IF (BUILD_DOCUMENTATION)
  OPTION (DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
         "Download the VTK tag file." ON)
  MARK_AS_ADVANCED (
    DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
  )

  #
  # Configure the script and the doxyfile, then add target
  #
  CONFIGURE_FILE(
    ${VTKLOCAL_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
    ${VTKLOCAL_BINARY_DIR}/Utilities/Doxygen/doxyfile)

  CONFIGURE_FILE(
    ${VTKLOCAL_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
    ${VTKLOCAL_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

  ADD_CUSTOM_TARGET(vtkLocalDoxygenDoc
    ${BASH}
    ${VTKLOCAL_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

ENDIF (BUILD_DOCUMENTATION)
