#-----------------------------------------------------------------------------
# Macro to compile VTK's CMake extension commands.
# The arguments are the source and build tree locations for the loaded
# commands project, and the variable in which success/failure should
# be stored.
MACRO(VTK_COMPILE_CMAKE_EXTENSIONS source_dir build_dir result_var)
  IF(COMMAND VTK_MAKE_INSTANTIATOR2)
  ELSE(COMMAND VTK_MAKE_INSTANTIATOR2)
    MESSAGE(STATUS "Compiling VTK CMake commands")
    TRY_COMPILE("${result_var}" "${build_dir}" "${source_dir}"
                VTK_LOADED_COMMANDS
                OUTPUT_VARIABLE VTK_COMPILE_CMAKE_EXTENSIONS_OUTPUT)
    IF("${result_var}")
      MESSAGE(STATUS "Compiling VTK CMake commands - done")
    ELSE("${result_var}")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeError.log 
        "Building of VTK extensions failed with the following output:\n"
        "${VTK_COMPILE_CMAKE_EXTENSIONS_OUTPUT}\n" APPEND)
      MESSAGE(FATAL_ERROR "Compiling VTK CMake commands - failed")
    ENDIF("${result_var}")
  ENDIF(COMMAND VTK_MAKE_INSTANTIATOR2)
ENDMACRO(VTK_COMPILE_CMAKE_EXTENSIONS)
