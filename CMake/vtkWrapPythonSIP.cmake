#
# a cmake implementation of the Wrap Python SIP command
# it creates bridge code for mixing vtkWrapPython code with SIP code
#


# VTK_WRAP_PYTHON_SIP takes a list of vtk source files to create sip bindings for
# it creates .sip files from a template and then creates .cxx files from those
# the .cxx files are returned from this macro
FUNCTION(VTK_WRAP_PYTHON_SIP KIT SRC_LIST_NAME SOURCES)
  IF(NOT SIP_EXECUTABLE)
    MESSAGE(SEND_ERROR "SIP_EXECUTABLE not specified when calling VTK_WRAP_PYTHON_SIP")
  ENDIF()

  # the name of the module

  SET(SIP_MOD vtk${KIT}PythonSIP)
  SET(SIP_OUT_SRCS)
  SET(SIP_IN_SRCS)
  SET(VTK_WRAPPER_INIT_DATA "%Module vtk.${SIP_MOD} 0\n\n")

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
      ELSE ()
        SET(TMP_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TMP_FILENAME}.h)
      ENDIF ()

      # new source file is namePython.cxx, add to resulting list
      SET(sip_output_src sip${SIP_MOD}${TMP_FILENAME}.cpp)
      SET(SIP_OUT_SRCS ${SIP_OUT_SRCS} ${sip_output_src})

      SET(VTK_CLASS ${TMP_FILENAME})
      SET(sip_input_src ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.sip)
      SET(SIP_IN_SRCS ${SIP_IN_SRCS} ${sip_input_src})
      CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkWrapPython.sip.in ${sip_input_src} @ONLY)

      SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA}%Include ${TMP_FILENAME}Python.sip\n")

    ENDIF ()

  ENDFOREACH()

  # finish the data file for the init file
  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${CMAKE_CURRENT_BINARY_DIR}/${SIP_MOD}.sip
    @ONLY
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

  SET(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${SIP_OUT_SRCS} PARENT_SCOPE)

ENDFUNCTION()

# create sip module library
# one is created for use within a VTK build, and another is created for use in an install tree
# all files passed in WRAP_SRCS are wrapped with sip/vtk bridge code
function(VTK_CREATE_SIP_MODULE KIT WRAP_SRCS)
  IF(NOT SIP_EXECUTABLE)
    MESSAGE(SEND_ERROR "SIP_EXECUTABLE not set.")
  ELSE()
    INCLUDE_DIRECTORIES(${SIP_INCLUDE_DIR})
    VTK_WRAP_PYTHON_SIP(${KIT} KitPythonSIP_SRCS "${WRAP_SRCS}")
    VTK_ADD_LIBRARY(vtk${KIT}PythonSIP MODULE ${KitPythonSIP_SRCS})
    SET_TARGET_PROPERTIES(vtk${KIT}PythonSIP PROPERTIES PREFIX "" SKIP_BUILD_RPATH 1)
    IF(WIN32 AND NOT CYGWIN)
      SET_TARGET_PROPERTIES(vtk${KIT}PythonSIP PROPERTIES SUFFIX ".pyd")
    ENDIF()
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonSIP vtk${KIT}PythonD)
    get_target_property(lib_loc vtk${KIT}PythonSIP LOCATION)
    ADD_CUSTOM_COMMAND(TARGET vtk${KIT}PythonSIP POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${lib_loc}" "${VTK_BUILD_PYTHON_MODULE_DIR}/vtk/"
      )

    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL(TARGETS vtk${KIT}PythonSIP
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
        LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development)
    ENDIF()
  ENDIF()
endfunction()
