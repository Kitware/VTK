#
# Build the documentation
#

INCLUDE (${CMAKE_ROOT}/Modules/Documentation.cmake OPTIONAL)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

IF (BUILD_DOCUMENTATION)
  OPTION (DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
         "Download the VTK tag file." ON)
  MARK_AS_ADVANCED (
    DOCUMENTATION_DOWNLOAD_VTK_TAGFILE
  )

  #
  # Configure the script and the doxyfile, then add target
  #
  IF(NOT DOT_PATH)
    GET_FILENAME_COMPONENT(DOT_PATH ${DOT} PATH)
  ENDIF()

  CONFIGURE_FILE(
    ${VTKMY_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
    ${VTKMY_BINARY_DIR}/Utilities/Doxygen/doxyfile)

  CONFIGURE_FILE(
    ${VTKMY_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in
    ${VTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

  ADD_CUSTOM_TARGET(vtkMyDoxygenDoc
    ${BASH}
    ${VTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh)

ENDIF ()
