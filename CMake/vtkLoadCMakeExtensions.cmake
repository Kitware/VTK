#-----------------------------------------------------------------------------
# Macro to load VTK's CMake extension commands.  The argument should
# be the directory containing the command modules.
MACRO(VTK_LOAD_SINGLE_CMAKE_EXTENSION name dir)
  IF(COMMAND ${name})
  ELSE(COMMAND ${name})
    LOAD_COMMAND(${name} ${dir} ${dir}/Debug)
  ENDIF(COMMAND ${name})
  IF(COMMAND ${name})
  ELSE(COMMAND ${name})
    MESSAGE(FATAL_ERROR "Loading VTK command ${name} - failed")
  ENDIF(COMMAND ${name})
ENDMACRO(VTK_LOAD_SINGLE_CMAKE_EXTENSION)

MACRO(VTK_LOAD_CMAKE_EXTENSIONS ext_dir)
  MESSAGE(STATUS "Loading VTK CMake commands")
  VTK_LOAD_SINGLE_CMAKE_EXTENSION(VTK_WRAP_TCL2 ${ext_dir})
  VTK_LOAD_SINGLE_CMAKE_EXTENSION(VTK_WRAP_PYTHON2 ${ext_dir})
  VTK_LOAD_SINGLE_CMAKE_EXTENSION(VTK_WRAP_JAVA2 ${ext_dir})
  VTK_LOAD_SINGLE_CMAKE_EXTENSION(VTK_MAKE_INSTANTIATOR2 ${ext_dir})
  VTK_LOAD_SINGLE_CMAKE_EXTENSION(VTK_GENERATE_JAVA_DEPENDENCIES ${ext_dir})
  MESSAGE(STATUS "Loading VTK CMake commands - done")
ENDMACRO(VTK_LOAD_CMAKE_EXTENSIONS)
