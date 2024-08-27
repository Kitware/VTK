#=========================================================================
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

include(GenerateExportHeader)

# Declare a library as needed to be installed
# supports the signature
#  token_install_library(target [DEPENDS <targets>])
# which allows you to export a target that has dependencies
function(token_install_library target)
  set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  if (NOT token_EXPORT_SET)
    set(token_EXPORT_SET "${PROJECT_NAME}")
  endif ()
  install(TARGETS ${target}
    EXPORT ${token_EXPORT_SET}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endfunction()

# Declare a target (library or executable) as needing installation.
# Usage:
#   token_install_target(target)
function(token_install_target target)
  token_install_library(${target})
endfunction()

#generate an export header and create an install target for it
function(token_export_header target file)
  token_get_kit_name(name dir_prefix)
  generate_export_header(${target} EXPORT_FILE_NAME ${file})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${file}  DESTINATION include/${PROJECT_NAME}/${token_VERSION}/${dir_prefix})
endfunction()

# Builds a source file and an executable that does nothing other than
# compile the given header files.
function(token_prepend_string prefix result)
  set(names ${ARGN})
  set(newNames "")
  foreach (name ${ARGN})
    if (IS_ABSOLUTE "${name}")
      set(newName "${name}")
    else ()
      set(newName "${prefix}/${name}")
    endif ()
    set(newNames ${newNames} ${newName})
  endforeach ()
  set(${result} ${newNames} PARENT_SCOPE)
endfunction()

set(_tokenMacros_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")
