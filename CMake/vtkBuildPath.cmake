# Attempt to build up the path/ld_library_path/python path needed to run VTK.
# On Windows simply executing the .bat file should be enough, on Linux/Mac the
# file can be sourced in the shell. You can also copy and paste the relevant
# parts into other files if preferred.
#
# Note: Now only setting the path to the latest configuration used (for MSVC/Xcode)

set(cfg_bit "")
if (CMAKE_CONFIGURATION_TYPES)
  set(cfg_bit ".$<CONFIGURATION>")
endif ()

if(WIN32)
  set(VTK_PATH_SHELL_SCRIPT "windows_path${cfg_bit}.bat")
  set(PATH_FORMAT "set xxx_path_var=xxx_add_path;%xxx_path_var%\r\n")
  set(PATH_VARIABLE "PATH")
  set(PATH_SEPARATOR ";")
elseif(UNIX)
  set(VTK_PATH_SHELL_SCRIPT "unix_path${cfg_bit}.sh")
  if(APPLE)
    set(DYLD "DYLD")
  else()
    set(DYLD "LD")
  endif()
  set(PATH_VARIABLE "${DYLD}_LIBRARY_PATH")
  set(PATH_SEPARATOR ":")
  set(PATH_FORMAT "export xxx_path_var=xxx_add_path:\${xxx_path_var}\n")
endif()

# set the script file name
set(PATH_FILENAME "${VTK_BINARY_DIR}/${VTK_PATH_SHELL_SCRIPT}")

set(cfg_subdir "")
if (CMAKE_CONFIGURATION_TYPES)
  set(cfg_subdir "/$<CONFIGURATION>")
endif ()

# FOR THE PATH VARIABLE
# replace the path to the executables
string(REPLACE "xxx_add_path" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}${cfg_subdir}" PATH_TEMP "${PATH_FORMAT}")
# replace the name of the platform-specific path environment variable
string(REPLACE "xxx_path_var" "${PATH_VARIABLE}" PATH_LINES "${PATH_TEMP}")

if(VTK_WRAP_PYTHON)
  # FOR THE PYTHONPATH VARIABLE, if PYTHON is wrapped
  # replace the path to the python-specific files
  string(REPLACE "xxx_add_path" "${VTK_BINARY_DIR}/Wrapping/Python${PATH_SEPARATOR}${CMAKE_LIBRARY_OUTPUT_DIRECTORY}${cfg_subdir}" PATH_TEMP "${PATH_FORMAT}")
  # replace pathvar by PYTHONPATH
  string(REPLACE "xxx_path_var" "PYTHONPATH" PATH_TEMP "${PATH_TEMP}")
  # apped the line to the file
  set(PATH_LINES "${PATH_LINES}${PATH_TEMP}")
endif()

# write to file
file(GENERATE
  OUTPUT  "${PATH_FILENAME}"
  CONTENT "${PATH_LINES}")
