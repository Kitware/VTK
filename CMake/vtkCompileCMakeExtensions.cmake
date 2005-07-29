#-----------------------------------------------------------------------------
# Macro to compile VTK's CMake extension commands.
# The arguments are the source and build tree locations for the loaded
# commands project, and the variable in which success/failure should
# be stored.
MACRO(VTK_COMPILE_CMAKE_EXTENSIONS source_dir build_dir result_var)
  # do we need the extensions
  SET (VTK_NEED_LOADED_COMMANDS 0)
  IF (VTK_WRAP_TCL OR VTK_WRAP_PYTHON)
    SET (VTK_NEED_LOADED_COMMANDS 1)
  ENDIF (VTK_WRAP_TCL OR VTK_WRAP_PYTHON)
  IF (VTK_WRAP_JAVA)
    SET (VTK_NEED_LOADED_COMMANDS 1)
  ENDIF (VTK_WRAP_JAVA)
  
  # if we need them
  IF (VTK_NEED_LOADED_COMMANDS)
    # if they are not already loaded
    IF(COMMAND VTK_WRAP_TCL2)
    ELSE(COMMAND VTK_WRAP_TCL2)
      MESSAGE(STATUS "Compiling VTK CMake commands")
      TRY_COMPILE("${result_var}" "${build_dir}" "${source_dir}"
        VTK_LOADED_COMMANDS      
        CMAKE_FLAGS -DVTK_BUILD_FROM_TRY_COMPILE:BOOL=TRUE
        OUTPUT_VARIABLE VTK_COMPILE_CMAKE_EXTENSIONS_OUTPUT)
      IF("${result_var}")
        MESSAGE(STATUS "Compiling VTK CMake commands - done")
      ELSE("${result_var}")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log 
          "Building of VTK extensions failed with the following output:\n"
          "${VTK_COMPILE_CMAKE_EXTENSIONS_OUTPUT}\n" APPEND)
        MESSAGE(FATAL_ERROR "Compiling VTK CMake commands - failed")
      ENDIF("${result_var}")
    ENDIF(COMMAND VTK_WRAP_TCL2)
  ENDIF (VTK_NEED_LOADED_COMMANDS)
ENDMACRO(VTK_COMPILE_CMAKE_EXTENSIONS)
