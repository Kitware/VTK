##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# Used to determine the version for Viskores source using "git describe", if git
# is found. On success sets the following variables in caller's scope:
#   ${var_prefix}_VERSION
#   ${var_prefix}_VERSION_MAJOR
#   ${var_prefix}_VERSION_MINOR
#   ${var_prefix}_VERSION_PATCH
#   ${var_prefix}_VERSION_PATCH_EXTRA
#   ${var_prefix}_VERSION_FULL
#   ${var_prefix}_VERSION_IS_RELEASE is true, if patch-extra is empty.
#
# If git is not found, or git describe cannot be run successfully, then these
# variables are left unchanged and a status message is printed.
#
# Arguments are:
#   source_dir : Source directory
#   git_command : git executable
#   var_prefix : prefix for variables e.g. "Viskores".
function(determine_version source_dir git_command var_prefix)
  if ("" STREQUAL "")
    # We are in an exported tarball and should use the shipped version
    # information. Just return here to avoid the warning message at the end of
    # this function.
    return ()
  elseif (NOT Viskores_GIT_DESCRIBE AND
          EXISTS ${git_command} AND
          EXISTS ${source_dir}/.git)
    execute_process(
      COMMAND ${git_command} describe
      WORKING_DIRECTORY ${source_dir}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE output
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE)
    if (NOT result EQUAL 0)
      # git describe failed (bad return code).
      set(output "")
    endif()
  else ()
    # note, output may be set to empty if Viskores_GIT_DESCRIBE is not defined.
    set(output "${Viskores_GIT_DESCRIBE}")
  endif()

  unset(tmp_VERSION)
  extract_version_components("${output}" tmp)
  if(DEFINED tmp_VERSION)
    if (NOT "${tmp_VERSION}" STREQUAL "${${var_prefix}_VERSION}")
      message(WARNING
        "Version from git (${tmp_VERSION}) disagrees with hard coded version (${${var_prefix}_VERSION}). Either update the git tags or version.txt.")
    endif()
    foreach(suffix VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH
                   VERSION_PATCH_EXTRA VERSION_FULL VERSION_IS_RELEASE)
      set(${var_prefix}_${suffix} ${tmp_${suffix}} PARENT_SCOPE)
    endforeach()
  else()
    message(STATUS
      "Could not use git to determine source version, using version ${${var_prefix}_VERSION_FULL}")
  endif()
endfunction()

# Extracts components from a version string. See determine_version() for usage.
function(extract_version_components version_string var_prefix)
  string(REGEX MATCH "^v?(([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*))$"
    version_matches "${version_string}")
  if(CMAKE_MATCH_0)
    # note, we don't use CMAKE_MATCH_0 for `full` since it may or may not have
    # the `v` prefix.
    set(full ${CMAKE_MATCH_1})
    set(major ${CMAKE_MATCH_2})
    set(minor ${CMAKE_MATCH_3})
    set(patch ${CMAKE_MATCH_4})
    set(patch_extra ${CMAKE_MATCH_5})

    set(${var_prefix}_VERSION "${major}.${minor}" PARENT_SCOPE)
    set(${var_prefix}_VERSION_MAJOR ${major} PARENT_SCOPE)
    set(${var_prefix}_VERSION_MINOR ${minor} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH ${patch} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH_EXTRA ${patch_extra} PARENT_SCOPE)
    set(${var_prefix}_VERSION_FULL ${full} PARENT_SCOPE)
    if("${major}.${minor}.${patch}" VERSION_EQUAL "${full}")
      set(${var_prefix}_VERSION_IS_RELEASE TRUE PARENT_SCOPE)
    else()
      set(${var_prefix}_VERSION_IS_RELEASE FALSE PARENT_SCOPE)
    endif()
  endif()
endfunction()
