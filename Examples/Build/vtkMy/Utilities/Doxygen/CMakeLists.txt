#
# Build the documentation
#

include(${CMAKE_ROOT}/Modules/Documentation.cmake OPTIONAL)

if(BUILD_DOCUMENTATION)

  #
  # The needed perl scripts are in Utilities/Doxygen in VTK_SOURCE_DIR
  #
  if(NOT DEFINED VTK_SOURCE_DIR)
    # go up three directories from vtkMy source directory
    get_filename_component(_tmp "${VTKMY_SOURCE_DIR}" DIRECTORY)
    get_filename_component(_tmp "${_tmp}" DIRECTORY)
    get_filename_component(_tmp "${_tmp}" DIRECTORY)
    set(VTK_SOURCE_DIR "${_tmp}" CACHE PATH "VTK Source Directory")
  endif()

  option(DOCUMENTATION_DOWNLOAD_VTK_TAGFILE "Download the VTK tag file." ON)
  mark_as_advanced(DOCUMENTATION_DOWNLOAD_VTK_TAGFILE)

  #
  # Configure the script and the doxyfile, then add target
  #
  if(DOT AND NOT DOT_PATH)
    get_filename_component(DOT_PATH "${DOT}" DIRECTORY)
  endif()

  configure_file(
    "${VTKMY_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in"
    "${VTKMY_BINARY_DIR}/Utilities/Doxygen/doxyfile")

  configure_file(
    "${VTKMY_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in"
    "${VTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh")

  add_custom_target(vtkMyDoxygenDoc
    "${BASH}"
    "${VTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh")

endif()
