#
# a cmake implementation of the Wrap Python SIP command
# it creates bridge code for mixing vtkWrapPython code with SIP code
#

MACRO(VTK_WRAP_PYTHON_SIP KIT SRC_LIST_NAME SOURCES)
  IF(NOT SIP_EXECUTABLE)
    MESSAGE(SEND_ERROR "SIP_EXECUTABLE not specified when calling VTK_WRAP_PYTHON_SIP")
  ENDIF(NOT SIP_EXECUTABLE)

  # the name of the module

  SET(SIP_MOD vtk${KIT}PythonSIP)
  SET(SIP_OUT_SRCS)
  SET(SIP_IN_SRCS)
  SET(VTK_WRAPPER_INIT_DATA "%Module ${SIP_MOD} 0\n\n")

  # For each class
  FOREACH(FILE ${SOURCES})
    # should we wrap the file?
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_SPECIAL ${FILE} WRAP_SPECIAL)

    # if we should wrap it
    IF (NOT TMP_WRAP_SPECIAL AND NOT TMP_WRAP_EXCLUDE)

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

      # new source file is namePython.cxx, add to resulting list
      SET(sip_output_src sip${SIP_MOD}${TMP_FILENAME}.cpp)
      SET(SIP_OUT_SRCS ${SIP_OUT_SRCS} ${sip_output_src})

      SET(VTK_CLASS ${TMP_FILENAME})
      SET(sip_input_src ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.sip)
      SET(SIP_IN_SRCS ${SIP_IN_SRCS} ${sip_input_src})
      CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkWrapPython.sip.in ${sip_input_src} @ONLY)

      SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA}%Include ${TMP_FILENAME}Python.sip\n")

    ENDIF (NOT TMP_WRAP_SPECIAL AND NOT TMP_WRAP_EXCLUDE)

  ENDFOREACH(FILE)

  # finish the data file for the init file
  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MOD}.sip
    COPY_ONLY
    )
  SET(SIP_IN_SRCS ${SIP_IN_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MOD}.sip)
  SET(SIP_OUT_SRCS ${SIP_OUT_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/sip${SIP_MOD}cmodule.cpp)

  ADD_CUSTOM_COMMAND(
    OUTPUT ${SIP_OUT_SRCS}
    DEPENDS ${SIP_EXECUTABLE} ${SIP_IN_SRCS}
    COMMAND ${SIP_EXECUTABLE}
    ARGS -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MOD}.sip
    COMMENT "Python SIP Wrapping - generating for vtk${KIT}"
    )

  SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${SIP_OUT_SRCS})

ENDMACRO(VTK_WRAP_PYTHON_SIP)
