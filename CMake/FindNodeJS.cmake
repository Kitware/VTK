#[==[
Provides the following variables:

  * `NodeJS_FOUND`: Whether NodeJS was found or not.
  * `NodeJS_INTERPRETER`: Path to the `node` interpreter.
#]==]
find_program (NodeJS_INTERPRETER
  NAMES node nodejs
  HINTS
    "$ENV{NODE_DIR}/bin"
    "$ENV{NODE_DIR}/" # On windows, node release binaries do not have bin.
  DOC
    "Node.js interpreter")

include (FindPackageHandleStandardArgs)

if (NodeJS_INTERPRETER)
  execute_process(COMMAND "${NodeJS_INTERPRETER}" --version
      OUTPUT_VARIABLE _nodejs_version
      RESULT_VARIABLE _nodejs_version_result)
  if (NOT _nodejs_version_result)
      string(REGEX MATCH "v([0-9]+)\\.([0-9]+)\\.([0-9]+)" _nodejs_version_match "${_nodejs_version}")
      set(_nodejs_version_major ${CMAKE_MATCH_1})
      set(_nodejs_version_minor ${CMAKE_MATCH_2})
      set(_nodejs_version_patch ${CMAKE_MATCH_3})
      set(_nodejs_version_string "${_nodejs_version_major}.${_nodejs_version_minor}.${_nodejs_version_patch}")
  endif ()
endif ()

find_package_handle_standard_args (NodeJS
  REQUIRED_VARS NodeJS_INTERPRETER
  VERSION_VAR _nodejs_version_string)
