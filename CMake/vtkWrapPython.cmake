#
# a cmake implementation of the Wrap Python command
#

MACRO(VTK_WRAP_PYTHON3 TARGET SRC_LIST_NAME SOURCES)
  IF(NOT VTK_WRAP_PYTHON_INIT_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_PYTHON_INIT_EXE not specified when calling VTK_WRAP_PYTHON3")
  ENDIF(NOT VTK_WRAP_PYTHON_INIT_EXE)
  IF(NOT VTK_WRAP_PYTHON_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_PYTHON_EXE not specified when calling VTK_WRAP_PYTHON3")
  ENDIF(NOT VTK_WRAP_PYTHON_EXE)
  # for new cmake use the new custom commands
  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)

    # start writing the input file for the init file
    SET(VTK_WRAPPER_INIT_DATA "${TARGET}")
    
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

        # add the info to the init file
        SET(VTK_WRAPPER_INIT_DATA
          "${VTK_WRAPPER_INIT_DATA}\n${TMP_FILENAME}")
        
        # new source file is namePython.cxx, add to resulting list
        SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} 
          ${TMP_FILENAME}Python.cxx)

        # add custom command to output
        ADD_CUSTOM_COMMAND(
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx
          DEPENDS ${VTK_WRAP_PYTHON_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT}
          COMMAND ${VTK_WRAP_PYTHON_EXE}
          ARGS ${TMP_INPUT} ${VTK_WRAP_HINTS} ${TMP_CONCRETE} 
          ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx
          COMMENT "Python Wrappings"
          )
        
      ENDIF (NOT TMP_WRAP_EXCLUDE)
    ENDFOREACH(FILE)
    
    # finish the data file for the init file        
    SET(dir ${CMAKE_CURRENT_SOURCE_DIR})
    IF(VTK_WRAP_PYTHON3_INIT_DIR)
      SET(dir ${VTK_WRAP_PYTHON3_INIT_DIR})
    ENDIF(VTK_WRAP_PYTHON3_INIT_DIR)
    CONFIGURE_FILE(
      ${dir}/vtkWrapperInit.data.in 
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
      COPY_ONLY
      IMMEDIATE
      )
    
    ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
      DEPENDS ${VTK_WRAP_PYTHON_INIT_EXE}
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
      COMMAND ${VTK_WRAP_PYTHON_INIT_EXE}
      ARGS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
      COMMENT "Python Wrapping Init"
      )
    
    # Create the Init File
    SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${TARGET}Init.cxx)

  ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)
    #otherwise use old loaded command
    VTK_WRAP_PYTHON2(${TARGET} 
      SOURCES ${SRC_LIST} ${SOURCES}
      )
  ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.6)  
ENDMACRO(VTK_WRAP_PYTHON3)
