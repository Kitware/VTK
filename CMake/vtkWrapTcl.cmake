#
# a cmake implementation of the Wrap Tcl command
# it takes an optional VERSION parameter that will be output
# to the .data file as VERSION ${VERSION}.
# vtkWrapTclInit will then recognize the VERSION keyword, extract the version
# and make sure the corresponding Tcl package is 'provided' with that version.
#

MACRO(VTK_WRAP_TCL2 TARGET)
  # convert to the WRAP3 signature
  SET(MODE SOURCE_LIST)
  SET(SOURCES)
  FOREACH(ARG ${ARGN})
    SET (MODE_CHANGED 0)
    IF ("${ARG}" MATCHES SOURCES)
      SET (MODE SOURCE_LIST)
      SET(MODE_CHANGED 1)
    ENDIF ("${ARG}" MATCHES SOURCES)
    IF ("${ARG}" MATCHES COMMANDS)
      SET (MODE COMMANDS)
      SET(MODE_CHANGED 1)
    ENDIF ("${ARG}" MATCHES COMMANDS)
    IF (NOT MODE_CHANGED)
      IF (MODE MATCHES SOURCES)
        SET(SOURCES ${SOURCES} ${ARG})
      ENDIF (MODE MATCHES SOURCES)
      IF (MODE MATCHES COMMANDS)
        SET(COMMANDS ${COMMANDS} ${ARG})
      ENDIF (MODE MATCHES COMMANDS)
      IF (MODE MATCHES SOURCE_LIST)
        SET(SOURCE_LIST_NAME "${ARG}")
        SET (MODE SOURCES)
      ENDIF (MODE MATCHES SOURCE_LIST)
    ENDIF (NOT MODE_CHANGED)
  ENDFOREACH(ARG)
  
  VTK_WRAP_TCL3(${TARGET} ${SOURCE_LIST_NAME} "${SOURCES}" "${COMMANDS}")
ENDMACRO(VTK_WRAP_TCL2)

MACRO(VTK_WRAP_TCL3 TARGET SRC_LIST_NAME SOURCES COMMANDS)
  IF(NOT VTK_WRAP_TCL_INIT_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_TCL_INIT_EXE not specified when calling VTK_WRAP_TCL3")
  ENDIF(NOT VTK_WRAP_TCL_INIT_EXE)
  IF(NOT VTK_WRAP_TCL_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_TCL_EXE not specified when calling VTK_WRAP_TCL3")
  ENDIF(NOT VTK_WRAP_TCL_EXE)

  # Initialize the custom target counter.
  IF(VTK_WRAP_TCL_NEED_CUSTOM_TARGETS)
    SET(VTK_WRAP_TCL_CUSTOM_COUNT "")
    SET(VTK_WRAP_TCL_CUSTOM_NAME ${TARGET})
    SET(VTK_WRAP_TCL_CUSTOM_LIST)
  ENDIF(VTK_WRAP_TCL_NEED_CUSTOM_TARGETS)
  
  # start writing the input file for the init file
  SET(VTK_WRAPPER_INIT_DATA "${TARGET}")
  IF (${ARGC} GREATER 4)
    SET(VTK_WRAPPER_INIT_DATA
      "${VTK_WRAPPER_INIT_DATA}\nVERSION ${ARGV4}")
  ENDIF (${ARGC} GREATER 4)
  
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
        # add the info to the init file
        SET(VTK_WRAPPER_INIT_DATA
          "${VTK_WRAPPER_INIT_DATA}\n${TMP_FILENAME}")
      ENDIF (TMP_ABSTRACT)
      
      # new source file is nameTcl.cxx, add to resulting list
      SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} 
        ${TMP_FILENAME}Tcl.cxx)
      
      # add custom command to output
      ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Tcl.cxx
        DEPENDS ${VTK_WRAP_TCL_EXE} ${VTK_WRAP_HINTS}
        MAIN_DEPENDENCY "${TMP_INPUT}"
        COMMAND ${VTK_WRAP_TCL_EXE}
        ARGS ${TMP_INPUT} ${VTK_WRAP_HINTS} ${TMP_CONCRETE} 
        ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Tcl.cxx
        COMMENT
        "Tcl Wrappings"
        )
      
      # Add this output to a custom target if needed.
      IF(VTK_WRAP_TCL_NEED_CUSTOM_TARGETS)
        SET(VTK_WRAP_TCL_CUSTOM_LIST ${VTK_WRAP_TCL_CUSTOM_LIST}
          ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Tcl.cxx)
        SET(VTK_WRAP_TCL_CUSTOM_COUNT ${VTK_WRAP_TCL_CUSTOM_COUNT}x)
        IF(VTK_WRAP_TCL_CUSTOM_COUNT MATCHES "^${VTK_WRAP_TCL_CUSTOM_LIMIT}$")
          SET(VTK_WRAP_TCL_CUSTOM_NAME ${VTK_WRAP_TCL_CUSTOM_NAME}Hack)
          ADD_CUSTOM_TARGET(${VTK_WRAP_TCL_CUSTOM_NAME} DEPENDS ${VTK_WRAP_TCL_CUSTOM_LIST})
          SET(KIT_TCL_DEPS ${VTK_WRAP_TCL_CUSTOM_NAME})
          SET(VTK_WRAP_TCL_CUSTOM_LIST)
          SET(VTK_WRAP_TCL_CUSTOM_COUNT)
        ENDIF(VTK_WRAP_TCL_CUSTOM_COUNT MATCHES "^${VTK_WRAP_TCL_CUSTOM_LIMIT}$")
      ENDIF(VTK_WRAP_TCL_NEED_CUSTOM_TARGETS)
    ENDIF (NOT TMP_WRAP_EXCLUDE)
  ENDFOREACH(FILE)
  
  # finish the data file for the init file        
  FOREACH(CMD ${COMMANDS})
    SET(VTK_WRAPPER_INIT_DATA
      "${VTK_WRAPPER_INIT_DATA}\nCOMMAND ${CMD}")
  ENDFOREACH(CMD ${COMMANDS})
  
  SET(dir ${CMAKE_CURRENT_SOURCE_DIR})
  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in 
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COPY_ONLY
    IMMEDIATE
    )
  
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    DEPENDS ${VTK_WRAP_TCL_INIT_EXE}
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COMMAND ${VTK_WRAP_TCL_INIT_EXE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    COMMENT "Tcl Wrapping Init"
    )
  
  # Create the Init File
  SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${TARGET}Init.cxx)
ENDMACRO(VTK_WRAP_TCL3)

# VS 6 does not like needing to run a huge number of custom commands
# when building a single target.  Generate some extra custom targets
# that run the custom commands before the main target is built.  This
# is a hack to work-around the limitation.  The test to enable it is
# done here since it does not need to be done for every macro
# invocation.
IF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")
  SET(VTK_WRAP_TCL_NEED_CUSTOM_TARGETS 1)
  SET(VTK_WRAP_TCL_CUSTOM_LIMIT x)
  # Limit the number of custom commands in each target
  # to 2^7.
  FOREACH(t 1 2 3 4 5 6 7)
    SET(VTK_WRAP_TCL_CUSTOM_LIMIT
      ${VTK_WRAP_TCL_CUSTOM_LIMIT}${VTK_WRAP_TCL_CUSTOM_LIMIT})
  ENDFOREACH(t)
ENDIF(CMAKE_GENERATOR MATCHES "^Visual Studio 6$")
