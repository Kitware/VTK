#
# Build the documentation
#
find_package(Doxygen REQUIRED)
find_package(Perl REQUIRED)
find_package(HTMLHelp)

option(DOXYGEN_SHORT_NAMES "Build Doxygen of VTK using short file names" OFF)
option(DOXYGEN_KEEP_TEMP "Do not delete the temporary Doxygen headers" OFF)
mark_as_advanced(DOXYGEN_SHORT_NAMES DOXYGEN_KEEP_TEMP)
if(DOXYGEN_SHORT_NAMES)
  set(DOXYGEN_SHORT_NAMES YES)
else()
  set(DOXYGEN_SHORT_NAMES NO)
endif()

#
# Configure the script and the doxyfile, then add target
#
set(HAVE_DOT_YESNO NO)
if(DOT)
  set(HAVE_DOT_YESNO YES)
  if(NOT DOT_PATH)
    get_filename_component(DOT_PATH ${DOT} PATH)
  endif()
endif()

# Build up a list of all module source directories. Note that this should be
# all source directories and so does not use the normal variables.
unset(VTK_MODULE_DIRS_DOXYGEN)
file(GLOB src RELATIVE "${VTK_SOURCE_DIR}" "${VTK_SOURCE_DIR}/*/*/module.cmake")
foreach(module ${src})
  get_filename_component(module_BASE ${module} PATH)
  if("${module_BASE}" MATCHES "^Utilities" OR ${module_BASE} MATCHES "^Third")
    # Skip the utilities and third parties directories.
  else()
    # Use both the source and binary directories, this ensures that
    # generated files will be included, but they may then be architecture,
    # build configuration and/or compiler specific. All source directories
    # are included whether the module is enabled or not.
    set(VTK_MODULE_DIRS_DOXYGEN
      "${VTK_MODULE_DIRS_DOXYGEN}
  \"${VTK_SOURCE_DIR}/${module_BASE}\"
  \"${VTK_BINARY_DIR}/${module_BASE}\"")
  endif()
endforeach()

configure_file(
  ${VTK_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in
  ${VTK_BINARY_DIR}/Utilities/Doxygen/doxyfile)

configure_file(
  ${VTK_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.cmake.in
  ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake
  @ONLY)

add_custom_target(DoxygenDoc
  ${CMAKE_COMMAND}
  -P ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake
  DEPENDS ${VTK_BINARY_DIR}/Utilities/Doxygen/doc_makeall.cmake)

if(NOT VTK_INSTALL_NO_DOCUMENTATION)
  macro(__vtk_install_documentation_files glob)
    file(GLOB __files "${CMAKE_CURRENT_SOURCE_DIR}/${glob}")
    install(FILES ${__files}
      DESTINATION ${VTK_INSTALL_DOXYGEN_DIR}
      COMPONENT Development)
  endmacro()
  __vtk_install_documentation_files("*.css")
  __vtk_install_documentation_files("*.gif")
  __vtk_install_documentation_files("*.html")
  __vtk_install_documentation_files("*.pl")
  __vtk_install_documentation_files("*.stop")
  install(FILES doc_readme.txt
    DESTINATION ${VTK_INSTALL_DOXYGEN_DIR}
    COMPONENT Development)
endif()
