# Function to fetch remote modules.

# Helper to perform the initial git clone and checkout.
function(_git_clone git_executable git_repository git_tag module_dir)
  execute_process(
    COMMAND "${git_executable}" clone "${git_repository}" "${module_dir}"
    RESULT_VARIABLE error_code
    OUTPUT_QUIET
    ERROR_VARIABLE git_clone_error
    )
  if(error_code)
    message(FATAL_ERROR "Failed to clone repository: '${git_repository}' Clone error is: '${git_clone_error}'")
  Endif()

  execute_process(
    COMMAND "${git_executable}" checkout ${git_tag}
    WORKING_DIRECTORY "${module_dir}"
    RESULT_VARIABLE error_code
    OUTPUT_QUIET
    ERROR_QUIET
    )
  if(error_code)
    message(FATAL_ERROR "Failed to checkout tag: '${git_tag}'")
  endif()

  execute_process(
    COMMAND "${git_executable}" submodule init
    WORKING_DIRECTORY "${module_dir}"
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Failed to init submodules in: '${module_dir}'")
  endif()

  execute_process(
    COMMAND "${git_executable}" submodule update --recursive
    WORKING_DIRECTORY "${module_dir}"
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Failed to update submodules in: '${module_dir}'")
  endif()
endfunction()

# Helper to perform a git update.  Checks the current Git revision against the
# desired revision and only performs a fetch and checkout if needed.
function(_git_update git_executable git_repository git_tag module_dir)
  execute_process(
    COMMAND "${git_executable}" rev-parse --verify ${git_tag}
    WORKING_DIRECTORY "${module_dir}"
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE tag_hash
    )
  if(error_code)
    message(FATAL_ERROR "Failed to get the hash for tag '${module_dir}'")
  endif()
  execute_process(
    COMMAND "${git_executable}" rev-parse --verify HEAD
    WORKING_DIRECTORY "${module_dir}"
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE head_hash
    )
  if(error_code)
    message(FATAL_ERROR "Failed to get the hash for ${git_repository} HEAD")
  endif()

  # Is the hash checkout out that we want?
  if(NOT (tag_hash STREQUAL head_hash))
    execute_process(
      COMMAND "${git_executable}" fetch "${git_repository}"
      WORKING_DIRECTORY "${module_dir}"
      RESULT_VARIABLE error_code
      )
    if(error_code)
      message(FATAL_ERROR "Failed to fetch repository '${git_repository}'")
    endif()

    execute_process(
      COMMAND "${git_executable}" checkout ${git_tag}
      WORKING_DIRECTORY "${module_dir}"
      RESULT_VARIABLE error_code
      )
    if(error_code)
      message(FATAL_ERROR "Failed to checkout tag: '${git_tag}'")
    endif()

    execute_process(
      COMMAND "${git_executable}" submodule update --recursive
      WORKING_DIRECTORY "${module_dir}"
      RESULT_VARIABLE error_code
      )
    if(error_code)
      message(FATAL_ERROR "Failed to update submodules in: '${module_dir}'")
    endif()
  endif()
endfunction()

# Helper function to fetch a module stored in a Git repository.
# Git fetches are only performed when required.
function(_fetch_with_git git_executable git_repository git_tag module_dir)
  if("${git_tag}" STREQUAL "" OR "${git_repository}" STREQUAL "")
    message(FATAL_ERROR "Tag or repository for git checkout should not be empty.")
  endif()

  # If we don't have a clone yet.
  if(NOT EXISTS "${module_dir}")
    _git_clone("${git_executable}" "${git_repository}" "${git_tag}" "${module_dir}")
    message(STATUS " The remote module: ${git_repository} is cloned into the directory ${module_dir}")
  else() # We already have a clone, but we need to check that it has the right revision.
    _git_update("${git_executable}" "${git_repository}" "${git_tag}" "${module_dir}")
  endif()
endfunction()

# Download and turn on a remote module.
#
# The module CMake option is created: Module_${module_name}, which defaults to OFF.
# Once set to ON, the module is downloaded into the Remote module group.
#
# A module name and description are required.  The description will show up in
# the CMake user interface.
#
# The following options are currently supported:
#    [GIT_REPOSITORY url]        # URL of git repo
#    [GIT_TAG tag]               # Git branch name, commit id or tag
function(vtk_fetch_module _name _description)
  option(Module_${_name} "${_description}" OFF)
  mark_as_advanced(Module_${_name})


  # Fetch_$_remote_module} is deprecated. To maintain backward compatibility:
  if(Fetch_${_name})
    message(WARNING "Fetch_${_name} is deprecated, please use Module_${_name} to download and enable the remote module.")
    set(Module_${_name} ON CACHE FORCE "${_description}")
  endif()

  if(Module_${_name})
    vtk_download_attempt_check(Module_${_name})
    include(CMakeParseArguments)
    cmake_parse_arguments(_fetch_options "" "GIT_REPOSITORY;GIT_TAG" "" ${ARGN})
    find_package(Git)
    if(NOT GIT_EXECUTABLE)
      message(FATAL_ERROR "error: could not find git for clone of ${_name}")
    endif()
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" --version
      OUTPUT_VARIABLE ov
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string(REGEX REPLACE "^git version (.+)$" "\\1" _version "${ov}")
    if("${_version}" VERSION_LESS 1.6.6)
      message(FATAL_ERROR "Git version 1.6.6 or later is required.")
    endif()
    _fetch_with_git("${GIT_EXECUTABLE}"
      "${_fetch_options_GIT_REPOSITORY}"
      "${_fetch_options_GIT_TAG}"
      "${VTK_SOURCE_DIR}/Remote/${_name}"
      )
  endif()
endfunction()
