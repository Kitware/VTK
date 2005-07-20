#
# a cmake implementation of the Wrap Java command
#

MACRO(VTK_WRAP_JAVA3 TARGET SRC_LIST_NAME SOURCES)
  IF(NOT VTK_PARSE_JAVA_EXE)
    MESSAGE(SEND_ERROR "VTK_PARSE_JAVA_EXE not specified when calling VTK_WRAP_JAVA3")
  ENDIF(NOT VTK_PARSE_JAVA_EXE)
  IF(NOT VTK_WRAP_JAVA_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_JAVA_EXE not specified when calling VTK_WRAP_JAVA3")
  ENDIF(NOT VTK_WRAP_JAVA_EXE)
  # for new cmake use the new custom commands
  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)

    # Initialize the custom target counter.
    IF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)
      SET(VTK_WRAP_JAVA_CUSTOM_COUNT "")
      SET(VTK_WRAP_JAVA_CUSTOM_NAME ${TARGET})
      SET(VTK_WRAP_JAVA_CUSTOM_LIST)
    ENDIF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)

    SET(VTK_JAVA_DEPENDENCIES)
    SET(VTK_JAVA_DEPENDENCIES_FILE)
    # For each class
    FOREACH(FILE ${SOURCES})
      # should we wrap the file?
      GET_SOURCE_FILE_PROPERTY(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
      
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
        
        # is it abstract?
        GET_SOURCE_FILE_PROPERTY(TMP_ABSTRACT ${FILE} ABSTRACT)
        IF (TMP_ABSTRACT)
          SET(TMP_CONCRETE 0)
        ELSE (TMP_ABSTRACT)
          SET(TMP_CONCRETE 1)
        ENDIF (TMP_ABSTRACT)
        
        # new source file is nameJava.cxx, add to resulting list
        SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} 
          ${TMP_FILENAME}Java.cxx)

        # add custom command to output
        ADD_CUSTOM_COMMAND(
          OUTPUT ${VTK_JAVA_HOME}/${TMP_FILENAME}.java
          DEPENDS ${VTK_PARSE_JAVA_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT}
          COMMAND ${VTK_PARSE_JAVA_EXE}
          ARGS ${TMP_INPUT} ${VTK_WRAP_HINTS} ${TMP_CONCRETE} 
          ${VTK_JAVA_HOME}/${TMP_FILENAME}.java
          COMMENT "Java Wrappings"
          )
        
        # add custom command to output
        ADD_CUSTOM_COMMAND(
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Java.cxx
          DEPENDS ${VTK_WRAP_JAVA_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT}
          COMMAND ${VTK_WRAP_JAVA_EXE}
          ARGS ${TMP_INPUT} ${VTK_WRAP_HINTS} ${TMP_CONCRETE} 
          ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Java.cxx
          COMMENT "Java Wrappings"
          )

        SET(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES} "${VTK_JAVA_HOME}/${TMP_FILENAME}.java")
        SET(VTK_JAVA_DEPENDENCIES_FILE
          "${VTK_JAVA_DEPENDENCIES_FILE}\n  \"${VTK_JAVA_HOME}/${TMP_FILENAME}.java\"")

        # Add this output to a custom target if needed.
        IF(VTK_WRAP_JAVA_NEED_CUSTOM_TARGETS)
          SET(VTK_WRAP_JAVA_CUSTOM_LIST ${VTK_WRAP_JAVA_CUSTOM_LIST}
            ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Java.cxx
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
      ENDIF (NOT TMP_WRAP_EXCLUDE)
    ENDFOREACH(FILE)

    ADD_CUSTOM_TARGET("${TARGET}JavaClasses" ALL DEPENDS ${VTK_JAVA_DEPENDENCIES})
    SET(dir ${CMAKE_CURRENT_SOURCE_DIR})
    IF(VTK_WRAP_JAVA3_INIT_DIR)
      SET(dir ${VTK_WRAP_JAVA3_INIT_DIR})
    ENDIF(VTK_WRAP_JAVA3_INIT_DIR)
    CONFIGURE_FILE("${dir}/JavaDependencies.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/JavaDependencies.cmake" IMMEDIATE @ONLY)
    
  ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)
    #otherwise use old loaded command
    VTK_WRAP_JAVA2(${TARGET} 
      SOURCES ${SRC_LIST} ${SOURCES}
      )
  ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)  
ENDMACRO(VTK_WRAP_JAVA3)

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
