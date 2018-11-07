file(GLOB cmake_version_backports
  LIST_DIRECTORIES true
  RELATIVE "${CMAKE_CURRENT_LIST_DIR}/patches"
  "${CMAKE_CURRENT_LIST_DIR}/patches/*")

foreach (cmake_version_backport IN LISTS cmake_version_backports)
  if (NOT IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/patches/${cmake_version_backport}")
    continue ()
  endif ()
  if (CMAKE_VERSION VERSION_LESS "${cmake_version_backport}")
    list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/patches/${cmake_version_backport}")
  endif ()
endforeach ()
