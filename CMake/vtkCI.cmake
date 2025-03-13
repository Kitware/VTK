if (NOT DEFINED ENV{CI})
  return ()
endif ()

if (DEFINED ENV{GITLAB_CI})
  message(STATUS
    "GitLab CI Pipeline: $ENV{CI_PIPELINE_URL}")
  message(STATUS
    "GitLab CI Job: $ENV{CI_JOB_URL}")
  if (DEFINED ENV{CI_MERGE_REQUEST_PROJECT_URL})
    message(STATUS
      "GitLab Merge Request: $ENV{CI_MERGE_REQUEST_PROJECT_URL}/-/merge_requests/$ENV{CI_MERGE_REQUEST_IID}")
  endif ()
  if (DEFINED ENV{CI_PIPELINE_SCHEDULE_DESCRIPTION})
    message(STATUS
      "GitLab Scheduled Pipeline: $ENV{CI_PIPELINE_SCHEDULE_DESCRIPTION}")
  endif ()
else ()
  message(STATUS
    "CI detected, but no known backend")
endif ()
