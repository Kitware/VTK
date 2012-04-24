# Attempt to build up the path/ld_library_path/python path needed to run VTK.
# On Windows simply executing the .bat file should be enough, on Linux/Mac the
# file can be sourced in the shell. You can also copy and paste the relevant
# parts into other files if preferred.
#
# Note: on Windows Debug and Release are added, if another build type is
# used, it would need to be added to the PATH too.

set(VTK_PYTHONPATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

if(WIN32)
  list(APPEND VTK_PYTHONPATH
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug"
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release")
endif()

set(VTK_LIBRARY_PATH
  "${VTK_PYTHONPATH}")

if(WIN32)
  file(WRITE "${VTK_BINARY_DIR}/windows_path.bat"
    "set PATH=${VTK_LIBRARY_PATH};%PATH%
    set PYTHONPATH=${VTK_PYTHONPATH};%PYTHONPATH%")
elseif(UNIX)
  # Replace the semicolons with colons for Unix operating systems
  string(REPLACE ";" ":" VTK_LIBRARY_PATH "${VTK_LIBRARY_PATH}")
  string(REPLACE ";" ":" VTK_PYTHONPATH "${VTK_PYTHONPATH}")
  if(APPLE)
    set(DYLD "DYLD")
  else()
    set(DYLD "LD")
  endif()
  file(WRITE "${VTK_BINARY_DIR}/unix_path.sh"
    "export ${DYLD}_LIBRARY_PATH=${VTK_LIBRARY_PATH}:\${${DYLD}_LIBRARY_PATH}
    export PYTHONPATH=${VTK_PYTHONPATH}:\${PYTHONPATH}\n")
endif()
