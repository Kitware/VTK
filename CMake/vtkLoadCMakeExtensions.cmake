#-----------------------------------------------------------------------------
# Create the VTK_LOAD_CMAKE_EXTENSIONS macro.
# The arguments are the source and build tree locations for the loaded
# commands project.  These directories must not overlap with your
# project's source or build trees, but they can be subdirectories not
# otherwise used.
MACRO(VTK_LOAD_CMAKE_EXTENSIONS SOURCE_DIR BUILD_DIR)
  IF (COMMAND VTK_MAKE_INSTANTIATOR2)
  ELSE (COMMAND VTK_MAKE_INSTANTIATOR2)
     MESSAGE(STATUS "Compiling VTK loaded commands")
     TRY_COMPILE(COMPILE_OK 
        ${BUILD_DIR}
        ${SOURCE_DIR}
        VTK_LOADED_COMMANDS
        OUTPUT_VARIABLE OUTPUT)
     IF (COMPILE_OK)
        MESSAGE(STATUS "Compiling VTK loaded commands - done")
        # load the four extra CMake commands 
        LOAD_COMMAND(VTK_WRAP_TCL2 ${BUILD_DIR} ${BUILD_DIR}/Debug)
        LOAD_COMMAND(VTK_WRAP_PYTHON2 ${BUILD_DIR} ${BUILD_DIR}/Debug)
        LOAD_COMMAND(VTK_WRAP_JAVA2 ${BUILD_DIR} ${BUILD_DIR}/Debug)
        LOAD_COMMAND(VTK_MAKE_INSTANTIATOR2 ${BUILD_DIR} ${BUILD_DIR}/Debug)
        LOAD_COMMAND(VTK_GENERATE_JAVA_DEPENDENCIES ${BUILD_DIR} ${BUILD_DIR}/Debug)
     ELSE (COMPILE_OK)
        MESSAGE(STATUS "Compiling VTK loaded commands - failed")
        MESSAGE("failed to compile VTK extensions to CMake")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeError.log 
        "Building of VTK extensions failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
     ENDIF (COMPILE_OK)
  ENDIF (COMMAND VTK_MAKE_INSTANTIATOR2)
  SET(VTK_USE_INSTANTIATOR_NEW "1")
ENDMACRO(VTK_LOAD_CMAKE_EXTENSIONS)
