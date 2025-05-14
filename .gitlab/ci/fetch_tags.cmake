cmake_minimum_required(VERSION 3.8)

# Find Git.
find_program(GIT_COMMAND NAMES git git.cmd)

# Check if we're in a merge request (nothing should be needed if not).
if ("$ENV{CI_MERGE_REQUEST_ID}" STREQUAL "")
  return ()
endif ()

if (NOT GIT_COMMAND)
  message(FATAL_ERROR
    "`git` was not found.")
endif ()

# Fetch tags from the target repo.
execute_process(
  COMMAND "${GIT_COMMAND}" fetch "$ENV{CI_MERGE_REQUEST_PROJECT_URL}" --tags
  RESULT_VARIABLE res
  ERROR_VARIABLE err)
# Ignore errors.

# Fetch tags from the source repo.
execute_process(
  COMMAND "${GIT_COMMAND}" fetch "$ENV{CI_MERGE_REQUEST_SOURCE_PROJECT_URL}" --tags
  RESULT_VARIABLE res
  ERROR_VARIABLE err)
# Ignore errors.
