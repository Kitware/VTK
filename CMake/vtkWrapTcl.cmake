#
# a cmake implementation of the Wrap Tcl command
# it takes an optional VERSION parameter that will be output
# to the .data file as VERSION ${VERSION}.
# vtkWrapTclInit will then recognize the VERSION keyword, extract the version
# and make sure the corresponding Tcl package is 'provided' with that version.
#

MACRO(VTK_WRAP_TCL3 TARGET SRC_LIST_NAME SOURCES COMMANDS)
  IF(NOT VTK_WRAP_TCL_INIT_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_TCL_INIT_EXE not specified when calling VTK_WRAP_TCL3")
  ENDIF(NOT VTK_WRAP_TCL_INIT_EXE)
  IF(NOT VTK_WRAP_TCL_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_TCL_EXE not specified when calling VTK_WRAP_TCL3")
  ENDIF(NOT VTK_WRAP_TCL_EXE)
  # for new cmake use the new custom commands
  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)

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
        
      ENDIF (NOT TMP_WRAP_EXCLUDE)
    ENDFOREACH(FILE)
    
    # finish the data file for the init file        
    FOREACH(CMD ${COMMANDS})
      SET(VTK_WRAPPER_INIT_DATA
        "${VTK_WRAPPER_INIT_DATA}\nCOMMAND ${CMD}")
    ENDFOREACH(CMD ${COMMANDS})

    SET(dir ${CMAKE_CURRENT_SOURCE_DIR})
    IF(VTK_WRAP_TCL3_INIT_DIR)
      SET(dir ${VTK_WRAP_TCL3_INIT_DIR})
    ENDIF(VTK_WRAP_TCL3_INIT_DIR)
    CONFIGURE_FILE(
      ${dir}/vtkWrapperInit.data.in 
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

  ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)
    #otherwise use old loaded command
    VTK_WRAP_TCL2(${TARGET} 
      SOURCES ${SRC_LIST} ${SOURCES}
      COMMANDS ${COMMANDS}
      )
  ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)  
ENDMACRO(VTK_WRAP_TCL3)
