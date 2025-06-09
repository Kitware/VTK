# VTK Common Dashboard Script
#
# This script contains basic dashboard driver code common to all
# clients.
#
# Put this script in a directory such as "~/Dashboards/Scripts" or
# "c:/Dashboards/Scripts".  Create a file next to this script, say
# 'my_dashboard.cmake', with code of the following form:
#
#   # Client maintainer: me@mydomain.net
#   set(CTEST_SITE "machine.site")
#   set(CTEST_BUILD_NAME "Platform-Compiler")
#   set(CTEST_CONFIGURATION_TYPE Debug)
#   set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
#   include(${CTEST_SCRIPT_DIRECTORY}/vtk_common.cmake)
#
# Then run a scheduled task (cron job) with a command line such as
#
#   ctest -S ~/Dashboards/Scripts/my_dashboard.cmake -V
#
# By default the source and build trees will be placed in the path
# "../My Tests/" relative to your script location.
#
# The following variables may be set before including this script
# to configure it:
#
#   dashboard_model           = Nightly | Experimental | Continuous
#   dashboard_track           = Optional track to submit dashboard to
#   dashboard_branch_type     = master (default) | release
#   dashboard_disable_loop    = For continuous dashboards, disable loop.
#   dashboard_root_name       = Change name of "My Tests" directory
#   dashboard_source_name     = Name of source directory (VTK)
#   dashboard_binary_name     = Name of binary directory (VTK-build)
#   dashboard_cache           = Initial CMakeCache.txt file content
#   dashboard_configure_args  = CMake configure args
#   dashboard_cvs_tag         = CVS tag to checkout (ex: VTK-5-6)
#   dashboard_do_coverage     = True to enable coverage (ex: gcov)
#   dashboard_do_memcheck     = True to enable memcheck (ex: valgrind)
#   dashboard_do_superbuild   = True to do a superbuild dashboard.
#   dashboard_no_test         = True to not run the tests
#   CTEST_UPDATE_COMMAND      = path to svn command-line client
#   CTEST_BUILD_FLAGS         = build tool arguments (ex: -j2)
#   CTEST_DASHBOARD_ROOT      = Where to put source and build trees
#   CTEST_TEST_CTEST          = Whether to run long CTestTest* tests
#   CTEST_TEST_TIMEOUT        = Per-test timeout length
#   CTEST_TEST_ARGS           = ctest_test args (ex: PARALLEL_LEVEL 4)
#   CMAKE_MAKE_PROGRAM        = Path to "make" tool to use
#
# These variables may also be set dependent on dashboard_branch setting
# Applicable when dashboard_branch == master
#   dashboard_store_name      = Name of ExternalData store (ExternalData)
#   VTK_USE_LARGE_DATA        = True to enable tests using "large" data
#
# Options to configure builds from experimental git repository:
#   dashboard_git_url      = Custom git clone url
#   dashboard_git_branch   = Custom remote branch to track
#   dashboard_git_crlf     = Value of core.autocrlf for repository
#
# For Makefile generators the script may be executed from an
# environment already configured to use the desired compilers.
# Alternatively the environment may be set at the top of the script:
#
#   set(ENV{CC}  /path/to/cc)   # C compiler
#   set(ENV{CXX} /path/to/cxx)  # C++ compiler
#   set(ENV{FC}  /path/to/fc)   # Fortran compiler (optional)
#   set(ENV{LD_LIBRARY_PATH} /path/to/vendor/lib) # (if necessary)

#=============================================================================
# Copyright 2010-2013 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CTEST_PROJECT_NAME VTK)
set(dashboard_user_home "$ENV{HOME}")

# prepare to do things appropriately for the chosen VTK branch
if (NOT DEFINED dashboard_branch_type)
  set(dashboard_branch_type "master")
endif()
if (dashboard_branch_type STREQUAL "master")
  if(NOT DEFINED dashboard_git_branch)
    if("${dashboard_model}" STREQUAL "Nightly")
      set(dashboard_git_branch nightly-master )
    else()
      set(dashboard_git_branch master)
    endif()
  endif()
endif()
if (dashboard_branch_type STREQUAL "release")
  set (dashboard_branch_type "release")
  if(NOT DEFINED dashboard_git_branch)
    set (dashboard_git_branch "release")
  endif()
endif()
if(dashboard_do_superbuild)
  set(dashboard_git_branch unified)
endif()

# Select the top dashboard directory.
if(NOT DEFINED dashboard_root_name)
  set(dashboard_root_name "My Tests")
endif()
if(NOT DEFINED CTEST_DASHBOARD_ROOT)
  get_filename_component(CTEST_DASHBOARD_ROOT "${CTEST_SCRIPT_DIRECTORY}/../${dashboard_root_name}" ABSOLUTE)
endif()

# Select the model (Nightly, Experimental, Continuous).
if(NOT DEFINED dashboard_model)
  set(dashboard_model Nightly)
endif()
if(NOT "${dashboard_model}" MATCHES "^(Nightly|Experimental|Continuous)$")
  message(FATAL_ERROR "dashboard_model must be Nightly, Experimental, or Continuous")
endif()

# Default to a Debug build.
if(NOT DEFINED CTEST_CONFIGURATION_TYPE AND DEFINED CTEST_BUILD_CONFIGURATION)
  set(CTEST_CONFIGURATION_TYPE ${CTEST_BUILD_CONFIGURATION})
endif()

if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
  set(CTEST_CONFIGURATION_TYPE Debug)
endif()

# Choose CTest reporting mode.
if(NOT "${CTEST_CMAKE_GENERATOR}" MATCHES "Make")
  # Launchers work only with Makefile generators.
  set(CTEST_USE_LAUNCHERS 0)
elseif(NOT DEFINED CTEST_USE_LAUNCHERS)
  # The setting is ignored by CTest < 2.8 so we need no version test.
  set(CTEST_USE_LAUNCHERS 1)
endif()

# Configure testing.
if(NOT DEFINED CTEST_TEST_CTEST)
  set(CTEST_TEST_CTEST 1)
endif()
if(NOT CTEST_TEST_TIMEOUT)
  set(CTEST_TEST_TIMEOUT 1500)
endif()

# Select Git source to use.
if(NOT DEFINED dashboard_git_url)
  if (dashboard_do_superbuild)
    set(dashboard_git_url "https://github.com/demarle/VTK-superbuild.git")
  else()
    set(dashboard_git_url "https://gitlab.kitware.com/vtk/vtk.git")
  endif()
endif()

if(NOT DEFINED dashboard_git_crlf)
  if(UNIX)
    set(dashboard_git_crlf false)
  else(UNIX)
    set(dashboard_git_crlf true)
  endif(UNIX)
endif()

# Look for a GIT command-line client.
if(NOT DEFINED CTEST_GIT_COMMAND)
  find_program(CTEST_GIT_COMMAND NAMES git git.exe git.cmd)
endif()

if(NOT EXISTS ${CTEST_GIT_COMMAND})
  message(FATAL_ERROR "No Git Found.")
endif()

# Select a source directory name.
if(NOT DEFINED CTEST_SOURCE_DIRECTORY)
  if(DEFINED dashboard_source_name)
    set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_source_name})
  else()
    set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/VTK)
  endif()
endif()

# Select a build directory name.
if(NOT DEFINED CTEST_BINARY_DIRECTORY)
  if(DEFINED dashboard_binary_name)
    set(CTEST_BINARY_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_binary_name})
  elseif(dashboard_do_superbuild)
    set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/VTKSuperbuild)
  else()
    set(CTEST_BINARY_DIRECTORY ${CTEST_SOURCE_DIRECTORY}-build)
  endif()
endif()

# Select a regression test data location
# Select a data store.
if(NOT DEFINED ExternalData_OBJECT_STORES)
  if(DEFINED dashboard_store_name)
    set(ExternalData_OBJECT_STORES ${CTEST_DASHBOARD_ROOT}/${dashboard_store_name})
  else()
    set(ExternalData_OBJECT_STORES ${CTEST_DASHBOARD_ROOT}/ExternalData)
  endif()
endif()

if(NOT DEFINED VTK_USE_LARGE_DATA)
  set(VTK_USE_LARGE_DATA ON)
endif()

# Delete source tree if it is incompatible with current VCS.
if(EXISTS ${CTEST_SOURCE_DIRECTORY})
  if(CTEST_GIT_COMMAND)
    if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git")
      set(vcs_refresh "because it is not managed by git.")
    endif()
  endif()
  if(vcs_refresh AND "${CTEST_SOURCE_DIRECTORY}" MATCHES "/VTK[^/]*")
    message("Deleting source tree\n  ${CTEST_SOURCE_DIRECTORY}\n${vcs_refresh}")
    file(REMOVE_RECURSE "${CTEST_SOURCE_DIRECTORY}")
  endif()
endif()

# Support initial checkout if necessary.
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}" AND NOT DEFINED CTEST_CHECKOUT_COMMAND)
  get_filename_component(_name "${CTEST_SOURCE_DIRECTORY}" NAME)
  if(CTEST_GIT_COMMAND)
    execute_process(COMMAND ${CTEST_GIT_COMMAND} --version OUTPUT_VARIABLE output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+(\\.g[0-9a-f]+)?)?" GIT_VERSION "${output}")
    if(NOT "${GIT_VERSION}" VERSION_LESS "1.6.5")
      # Have "git clone -b <branch>" option.
      set(git_branch_new "-b ${dashboard_git_branch}")
      set(git_branch_old)
    else()
      # No "git clone -b <branch>" option.
      set(git_branch_new)
      set(git_branch_old "-b ${dashboard_git_branch} origin/${dashboard_git_branch}")
    endif()

    # Generate an initial checkout script.
    set(ctest_checkout_script ${CTEST_DASHBOARD_ROOT}/${_name}-init.cmake)
    file(WRITE ${ctest_checkout_script} "# git repo init script for ${_name}
execute_process(
  COMMAND \"${CTEST_GIT_COMMAND}\" clone -n ${git_branch_new} \"${dashboard_git_url}\"
          \"${CTEST_SOURCE_DIRECTORY}\"
  )
if(EXISTS \"${CTEST_SOURCE_DIRECTORY}/.git\")
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" config core.autocrlf ${dashboard_git_crlf}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" checkout ${git_branch_old}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" submodule init
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" submodule update --
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
endif()
")
    set(CTEST_CHECKOUT_COMMAND "\"${CMAKE_COMMAND}\" -P \"${ctest_checkout_script}\"")
  endif()
  # CTest delayed initialization is broken, so we put the
  # CTestConfig.cmake info here.
  set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")
  set(CTEST_DROP_METHOD "http")
  set(CTEST_DROP_SITE "open.cdash.org")
  set(CTEST_DROP_LOCATION "/submit.php?project=VTK")
  set(CTEST_DROP_SITE_CDASH TRUE)
endif()

#-----------------------------------------------------------------------------

# Send the main script as a note.
list(APPEND CTEST_NOTES_FILES
  "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
  "${CMAKE_CURRENT_LIST_FILE}"
  )

# Check for required variables.
foreach(req
    CTEST_CMAKE_GENERATOR
    CTEST_SITE
    CTEST_BUILD_NAME
    )
  if(NOT DEFINED ${req})
    message(FATAL_ERROR "The containing script must set ${req}")
  endif()
endforeach(req)

# Print summary information.
foreach(v
    CTEST_SITE
    CTEST_BUILD_NAME
    CTEST_SOURCE_DIRECTORY
    CTEST_BINARY_DIRECTORY
    CTEST_CMAKE_GENERATOR
    CTEST_CONFIGURATION_TYPE
    CTEST_CVS_COMMAND
    CTEST_GIT_COMMAND
    CTEST_CHECKOUT_COMMAND
    CTEST_SCRIPT_DIRECTORY
    CTEST_USE_LAUNCHERS
    ExternalData_OBJECT_STORES
    )
  if (DEFINED ${v})
    set(vars "${vars}  ${v}=[${${v}}]\n")
  endif()
endforeach(v)
message("Dashboard script configuration:\n${vars}\n")

# Avoid non-ascii characters in tool output.
set(ENV{LC_ALL} C)

# Helper macro to write the initial cache.
macro(write_cache)
  set(cache_build_type "")
  set(cache_make_program "")
  set(data_options "
ExternalData_OBJECT_STORES:STRING=${ExternalData_OBJECT_STORES}
VTK_USE_LARGE_DATA:BOOL=${VTK_USE_LARGE_DATA}
")
  if(CTEST_CMAKE_GENERATOR MATCHES "Make")
    set(cache_build_type CMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE})
    if(CMAKE_MAKE_PROGRAM)
      set(cache_make_program CMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM})
    endif()
  endif()
  file(WRITE ${CTEST_BINARY_DIRECTORY}/CMakeCache.txt "
SITE:STRING=${CTEST_SITE}
BUILDNAME:STRING=${CTEST_BUILD_NAME}
CTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
DART_TESTING_TIMEOUT:STRING=${CTEST_TEST_TIMEOUT}
VTK_BUILD_TESTING:STRING=WANT
${data_options}
${cache_build_type}
${cache_make_program}
${dashboard_cache}
")
endmacro(write_cache)

# Start with a fresh build tree.
file(MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
if(NOT "${CTEST_SOURCE_DIRECTORY}" STREQUAL "${CTEST_BINARY_DIRECTORY}")
  message("Clearing build tree...")
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

set(dashboard_continuous 0)
if("${dashboard_model}" STREQUAL "Continuous")
  set(dashboard_continuous 1)
endif()

# CTest 2.6 crashes with message() after ctest_test.
macro(safe_message)
  if(NOT "${CMAKE_VERSION}" VERSION_LESS 2.8 OR NOT safe_message_skip)
    message(${ARGN})
  endif()
endmacro()

if(COMMAND dashboard_hook_init)
  dashboard_hook_init()
endif()

set(dashboard_done 0)
while(NOT dashboard_done)
  if(dashboard_continuous)
    set(START_TIME ${CTEST_ELAPSED_TIME})
  endif()
  set(ENV{HOME} "${dashboard_user_home}")

  # Start a new submission.
  if(COMMAND dashboard_hook_start)
    dashboard_hook_start()
  endif()
  if(dashboard_track)
    ctest_start(${dashboard_model} TRACK ${dashboard_track})
  else()
    ctest_start(${dashboard_model})
  endif()
  if(COMMAND dashboard_hook_started)
    dashboard_hook_started()
  endif()

  # Always build if the tree is fresh.
  set(dashboard_fresh 0)
  if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
    set(dashboard_fresh 1)
    safe_message("Starting fresh build...")
    write_cache()
  endif()

  ctest_update(RETURN_VALUE count)
  set(CTEST_CHECKOUT_COMMAND) # checkout on first iteration only
  safe_message("Found ${count} changed files")

  if(dashboard_fresh OR NOT dashboard_continuous OR count GREATER 0)

    ctest_configure(OPTIONS "${dashboard_configure_args}")

    ctest_submit(PARTS Update Configure Notes)
    ctest_read_custom_files(${CTEST_BINARY_DIRECTORY})

    if(COMMAND dashboard_hook_build)
      dashboard_hook_build()
    endif()
    ctest_build(APPEND)
    ctest_submit(PARTS Build)

    if(COMMAND dashboard_hook_test)
      dashboard_hook_test()
    endif()
    if(NOT dashboard_no_test)
      ctest_test(${CTEST_TEST_ARGS} APPEND)
      ctest_submit(PARTS Test)
    endif()
    set(safe_message_skip 1) # Block further messages

    if(dashboard_do_coverage)
      ctest_coverage()
      ctest_submit(PARTS Coverage)
    endif()

    if(dashboard_do_memcheck)
      ctest_memcheck(${CTEST_TEST_ARGS})
      ctest_submit(PARTS MemCheck)
    endif()

    if(dashboard_do_superbuild)
      # not sure if I can make platform tests here so just globbing
      # for all expected archive types.
      file(GLOB tar_balls ${CTEST_BINARY_DIRECTORY}/*.tar.gz)
      file(GLOB tar_balls_2 ${CTEST_BINARY_DIRECTORY}/*.tgz)
      file(GLOB dmgs ${CTEST_BINARY_DIRECTORY}/*.dmg)
      file(GLOB exes ${CTEST_BINARY_DIRECTORY}/*.exe)
      file(GLOB zips ${CTEST_BINARY_DIRECTORY}/*.zip)
      set (files ${tar_balls} ${tar_balls_2} ${dmgs} ${exes} ${zips})
      if (files)
        ctest_upload(FILES ${files})
        ctest_submit(PARTS Upload)
      endif()
    endif()

    if(COMMAND dashboard_hook_submit)
      dashboard_hook_submit()
    endif()
    if(COMMAND dashboard_hook_end)
      dashboard_hook_end()
    endif()
  endif()

  if(dashboard_continuous AND NOT dashboard_disable_loop)
    # Delay until at least 5 minutes past START_TIME
    ctest_sleep(${START_TIME} 300 ${CTEST_ELAPSED_TIME})
    if(${CTEST_ELAPSED_TIME} GREATER 57600)
      set(dashboard_done 1)
    endif()
  else()
    # Not continuous, so we are done.
    set(dashboard_done 1)
  endif()
endwhile()
