#[==[
Sets `vtk_tool_cache_directory` to the per-user directory where downloaded
developer tools (the Emscripten SDK, NodeJS) should live.

Tools are cached rather than dropped in a temporary directory because they run
to roughly a gigabyte and are reused across build trees; they are cached
per-user rather than in the source tree so that a source archive stays clean and
several checkouts share one copy.

The location follows platform convention and can be overridden wholesale with
the `VTK_TOOL_CACHE_DIR` environment variable:

  * Windows: `%LOCALAPPDATA%/vtk`
  * macOS:   `~/Library/Caches/vtk`
  * else:    `${XDG_CACHE_HOME:-~/.cache}/vtk`
#]==]

if (DEFINED ENV{VTK_TOOL_CACHE_DIR} AND NOT "$ENV{VTK_TOOL_CACHE_DIR}" STREQUAL "")
  file(TO_CMAKE_PATH "$ENV{VTK_TOOL_CACHE_DIR}" vtk_tool_cache_directory)
elseif (CMAKE_HOST_WIN32)
  if (DEFINED ENV{LOCALAPPDATA} AND NOT "$ENV{LOCALAPPDATA}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}" _vtk_tool_cache_base)
  elseif (DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}/AppData/Local" _vtk_tool_cache_base)
  endif ()
elseif (CMAKE_HOST_APPLE)
  if (DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{HOME}/Library/Caches" _vtk_tool_cache_base)
  endif ()
else ()
  if (DEFINED ENV{XDG_CACHE_HOME} AND NOT "$ENV{XDG_CACHE_HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{XDG_CACHE_HOME}" _vtk_tool_cache_base)
  elseif (DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{HOME}/.cache" _vtk_tool_cache_base)
  endif ()
endif ()

if (NOT DEFINED vtk_tool_cache_directory)
  if (NOT _vtk_tool_cache_base)
    message(FATAL_ERROR
      "Unable to determine a cache directory for downloaded tools. Set the "
      "VTK_TOOL_CACHE_DIR environment variable to a writable directory.")
  endif ()
  set(vtk_tool_cache_directory "${_vtk_tool_cache_base}/vtk")
endif ()

unset(_vtk_tool_cache_base)
