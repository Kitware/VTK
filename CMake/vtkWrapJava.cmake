#
# a cmake implementation of the Wrap Java command
#

MACRO(VTK_WRAP_JAVA2 TARGET SOURCE_LIST_NAME)
  # convert to the WRAP3 signature
  vtk_wrap_java3(${TARGET} ${SOURCE_LIST_NAME} "${ARGN}")
ENDMACRO(VTK_WRAP_JAVA2)

macro(vtk_wrap_java3 TARGET SRC_LIST_NAME SOURCES)
  if(NOT VTK_PARSE_JAVA_EXE)
    if(TARGET vtkParseJava)
      set(VTK_PARSE_JAVA_EXE vtkParseJava)
    else()
      message(SEND_ERROR
        "VTK_PARSE_JAVA_EXE not specified when calling VTK_WRAP_JAVA3")
    endif()
  endif()
  if(NOT VTK_WRAP_JAVA_EXE)
    if(TARGET vtkWrapJava)
      set(VTK_WRAP_JAVA_EXE vtkWrapJava)
    else()
      message(SEND_ERROR
        "VTK_WRAP_JAVA_EXE not specified when calling VTK_WRAP_JAVA3")
    endif()
  endif()

  IF(CMAKE_GENERATOR MATCHES "NMake Makefiles")
    SET(verbatim "")
    SET(quote "\"")
  ELSE(CMAKE_GENERATOR MATCHES "NMake Makefiles")
    SET(verbatim "VERBATIM")
    SET(quote "")
  ENDIF(CMAKE_GENERATOR MATCHES "NMake Makefiles")

  # Initialize the custom target counter.
  IF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)
    SET(VTK_WRAP_JAVA_CUSTOM_COUNT "")
    SET(VTK_WRAP_JAVA_CUSTOM_NAME ${TARGET})
    SET(VTK_WRAP_JAVA_CUSTOM_LIST)
  ENDIF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)

  # all the include directories
  if(VTK_WRAP_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  else()
    set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  endif()

  # collect the common wrapper-tool arguments
  set(_common_args)
  get_directory_property(_def_list DEFINITION COMPILE_DEFINITIONS)
  foreach(TMP_DEF ${_def_list})
    set(_common_args "${_common_args}-D${TMP_DEF}\n")
  endforeach()
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(_common_args "${_common_args}-I\"${INCLUDE_DIR}\"\n")
  endforeach()
  if(VTK_WRAP_HINTS)
    set(_common_args "${_common_args}--hints \"${VTK_WRAP_HINTS}\"\n")
  endif()
  if(KIT_HIERARCHY_FILE)
    set(_common_args "${_common_args}--types \"${KIT_HIERARCHY_FILE}\"\n")
  endif()

  # write wrapper-tool arguments to a file
  string(STRIP "${_common_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(_args_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.args)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${_args_file} @ONLY)

  SET(VTK_JAVA_DEPENDENCIES)
  SET(VTK_JAVA_DEPENDENCIES_FILE)

  # For each class
  FOREACH(FILE ${SOURCES})
    # should we wrap the file?
    get_source_file_property(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)

    # some wrapped files need to be compiled as objective C++
    get_source_file_property(TMP_WRAP_OBJC ${FILE} WRAP_JAVA_OBJC)

    # if we should wrap it
    IF (NOT TMP_WRAP_EXCLUDE)

      # what is the filename without the extension
      GET_FILENAME_COMPONENT(TMP_FILENAME ${FILE} NAME_WE)

      # the input file might be full path so handle that
      GET_FILENAME_COMPONENT(TMP_FILEPATH ${FILE} PATH)

      # compute the input filename
      IF (TMP_FILEPATH)
        SET(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h)
      ELSE (TMP_FILEPATH)
        SET(TMP_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TMP_FILENAME}.h)
      ENDIF (TMP_FILEPATH)

      # use ".mm" suffix if file must be compiled with objective C++
      IF(TMP_WRAP_OBJC)
        SET(TMP_WRAPPED_FILENAME ${TMP_FILENAME}Java.mm)
      ELSE(TMP_WRAP_OBJC)
        SET(TMP_WRAPPED_FILENAME ${TMP_FILENAME}Java.cxx)
      ENDIF(TMP_WRAP_OBJC)

      # new source file is nameJava.cxx, add to resulting list
      SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}}
        ${TMP_WRAPPED_FILENAME})

      # add custom command to output
      ADD_CUSTOM_COMMAND(
        OUTPUT ${VTK_JAVA_HOME}/${TMP_FILENAME}.java
        DEPENDS ${VTK_PARSE_JAVA_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT} ${_args_file}
        ${KIT_HIERARCHY_FILE}
        COMMAND ${VTK_PARSE_JAVA_EXE}
        ARGS
        "${quote}@${_args_file}${quote}"
        "-o" "${quote}${VTK_JAVA_HOME}/${TMP_FILENAME}.java${quote}"
        "${quote}${TMP_INPUT}${quote}"
        COMMENT "Java Wrappings - generating ${TMP_FILENAME}.java"
        )

      # add custom command to output
      ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_WRAPPED_FILENAME}
        DEPENDS ${VTK_WRAP_JAVA_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT} ${_args_file}
        ${KIT_HIERARCHY_FILE}
        COMMAND ${VTK_WRAP_JAVA_EXE}
        ARGS
        "${quote}@${_args_file}${quote}"
        "-o" "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TMP_WRAPPED_FILENAME}${quote}"
        "${quote}${TMP_INPUT}${quote}"
        COMMENT "Java Wrappings - generating ${TMP_WRAPPED_FILENAME}"
        )

      SET(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES} "${VTK_JAVA_HOME}/${TMP_FILENAME}.java")
      SET(VTK_JAVA_DEPENDENCIES_FILE
        "${VTK_JAVA_DEPENDENCIES_FILE}\n  \"${VTK_JAVA_HOME}/${TMP_FILENAME}.java\"")

      # Add this output to a custom target if needed.
      IF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)
        SET(VTK_WRAP_JAVA_CUSTOM_LIST ${VTK_WRAP_JAVA_CUSTOM_LIST}
          ${CMAKE_CURRENT_BINARY_DIR}/${TMP_WRAPPED_FILENAME}
          )
        SET(VTK_WRAP_JAVA_CUSTOM_COUNT ${VTK_WRAP_JAVA_CUSTOM_COUNT}x)
        IF(VTK_WRAP_JAVA_CUSTOM_COUNT MATCHES "^${VTK_WRAP_JAVA_CUSTOM_LIMIT}$")
          SET(VTK_WRAP_JAVA_CUSTOM_NAME ${VTK_WRAP_JAVA_CUSTOM_NAME}Hack)
          ADD_CUSTOM_TARGET(${VTK_WRAP_JAVA_CUSTOM_NAME} DEPENDS ${VTK_WRAP_JAVA_CUSTOM_LIST})
          SET(KIT_JAVA_DEPS ${VTK_WRAP_JAVA_CUSTOM_NAME})
          SET(VTK_WRAP_JAVA_CUSTOM_LIST)
          SET(VTK_WRAP_JAVA_CUSTOM_COUNT)
        ENDIF(VTK_WRAP_JAVA_CUSTOM_COUNT MATCHES "^${VTK_WRAP_JAVA_CUSTOM_LIMIT}$")
      ENDIF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)
    ENDIF ()
  ENDFOREACH(FILE)

  ADD_CUSTOM_TARGET("${TARGET}JavaClasses" ALL DEPENDS ${VTK_JAVA_DEPENDENCIES})
  SET(dir ${CMAKE_CURRENT_SOURCE_DIR})
  IF(VTK_WRAP_JAVA3_INIT_DIR)
    SET(dir ${VTK_WRAP_JAVA3_INIT_DIR})
  ENDIF(VTK_WRAP_JAVA3_INIT_DIR)
  CONFIGURE_FILE("${dir}/JavaDependencies.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/JavaDependencies.cmake" @ONLY)
endmacro()

# VS 6 does not like needing to run a huge number of custom commands
# when building a single target.  Generate some extra custom targets
# that run the custom commands before the main target is built.  This
# is a hack to work-around the limitation.  The test to enable it is
# done here since it does not need to be done for every macro
# invocation.
IF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")
  SET(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS 1)
  SET(VTK_WRAP_JAVA_CUSTOM_LIMIT x)
  # Limit the number of custom commands in each target
  # to 2^7.
  FOREACH(t 1 2 3 4 5 6 7)
    SET(VTK_WRAP_JAVA_CUSTOM_LIMIT
      ${VTK_WRAP_JAVA_CUSTOM_LIMIT}${VTK_WRAP_JAVA_CUSTOM_LIMIT})
  ENDFOREACH(t)
ENDIF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")
