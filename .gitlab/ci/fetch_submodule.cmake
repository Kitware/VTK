cmake_minimum_required(VERSION 2.8.8)

# Check if we're in a merge request (nothing should be needed if not).
if ("$ENV{CI_MERGE_REQUEST_ID}" STREQUAL "")
  return ()
endif ()

# Find Git.
find_program(GIT_COMMAND NAMES git git.cmd)

if (NOT GIT_COMMAND)
  message(FATAL_ERROR
    "`git` was not found.")
endif ()

# Get the remote URL for this submodule.
execute_process(
  COMMAND "${GIT_COMMAND}" remote get-url origin
  OUTPUT_VARIABLE remote_url
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (res)
  execute_process(
    COMMAND "${GIT_COMMAND}" config --get remote.origin.url
    OUTPUT_VARIABLE remote_url
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (res)
    message(FATAL_ERROR
      "Failed to get the remote url for the submodule: ${err}")
  endif ()
endif ()

# Take the "directory" of the source project path and append the "filename" of
# the origin's URL. We can assume that the user's fork will use the same
# repository name in the vast majority of cases.
get_filename_component(user_namespace "$ENV{CI_MERGE_REQUEST_SOURCE_PROJECT_PATH}" DIRECTORY)
get_filename_component(project_name "${remote_url}" NAME)
set(user_fork "$ENV{CI_SERVER_URL}/${user_namespace}/${project_name}")

# When accessing non-existent forks, sometimes the fetch call just blocks!
# Providing a bogus username and password overcomes that blocking.
string(REGEX REPLACE "^(http[s]*://)" "\\1buildbot:buildbot@" user_fork "${user_fork}")

# Use a remote per merge request (aid in debugging if needed).
set(remote "ci-fetch-$ENV{CI_MERGE_REQUEST_ID}")

message("Removing old 'remote', if any")
execute_process(
  COMMAND "${GIT_COMMAND}" remote rm "${remote}")
message("Adding 'remote' for ${user_fork}...")
execute_process(
  COMMAND "${GIT_COMMAND}" remote add "${remote}" "${user_fork}")
message("Fetching...")
execute_process(
  COMMAND "${GIT_COMMAND}" fetch "${remote}"
  RESULT_VARIABLE res)

if (res)
  message("Fetch failed, continuing...")
endif ()

message("Removing 'remote' for cleanup.")
execute_process(COMMAND "${GIT_COMMAND}" remote rm "${remote}")
