#-----------------------------------------------------------------------------
# Macro to load VTK's CMake extension commands.  The argument should
# be the directory containing the command modules.
MACRO(VTK_LOAD_CMAKE_EXTENSIONS ext_dir)
  IF(COMMAND VTK_MAKE_INSTANTIATOR2)
  ELSE(COMMAND VTK_MAKE_INSTANTIATOR2)
    MESSAGE(STATUS "Loading VTK CMake commands")
    LOAD_COMMAND(VTK_WRAP_TCL2 ${ext_dir} ${ext_dir}/Debug)
    LOAD_COMMAND(VTK_WRAP_PYTHON2 ${ext_dir} ${ext_dir}/Debug)
    LOAD_COMMAND(VTK_WRAP_JAVA2 ${ext_dir} ${ext_dir}/Debug)
    LOAD_COMMAND(VTK_MAKE_INSTANTIATOR2 ${ext_dir} ${ext_dir}/Debug)
    LOAD_COMMAND(VTK_GENERATE_JAVA_DEPENDENCIES ${ext_dir} ${ext_dir}/Debug)
    MESSAGE(STATUS "Loading VTK CMake commands - done")
  ENDIF(COMMAND VTK_MAKE_INSTANTIATOR2)
ENDMACRO(VTK_LOAD_CMAKE_EXTENSIONS)
