#
# a cmake macro to generate a text file with the class hierarchy
#
MACRO(VTK_WRAP_HIERARCHY TARGET OUTPUT_DIR SOURCES)
  IF(NOT VTK_WRAP_HIERARCHY_EXE)
    MESSAGE(SEND_ERROR "VTK_WRAP_HIERARCHY_EXE not specified when calling VTK_WRAP_HIERARCHY")
  ENDIF(NOT VTK_WRAP_HIERARCHY_EXE)

  # The shell into which nmake.exe executes the custom command has some issues
  # with mixing quoted and unquoted arguments :( Let's help.

  IF(CMAKE_GENERATOR MATCHES "NMake Makefiles")
    SET(verbatim "")
    SET(quote "\"")
  ELSE(CMAKE_GENERATOR MATCHES "NMake Makefiles")
    SET(verbatim "VERBATIM")
    SET(quote "")
  ENDIF(CMAKE_GENERATOR MATCHES "NMake Makefiles")

  # be bold and parse all files, not just indicated ones
  SET(IGNORE_WRAP_EXCLUDE ON)

  # list of all files to wrap
  SET(VTK_WRAPPER_INIT_DATA)

  # list of used files
  SET(INPUT_FILES)

  # For each class
  FOREACH(FILE ${SOURCES})
    # should we wrap the file?
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_SPECIAL ${FILE} WRAP_SPECIAL)

    # if we should wrap it
    IF (IGNORE_WRAP_EXCLUDE OR TMP_WRAP_SPECIAL OR NOT TMP_WRAP_EXCLUDE)
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

      IF(NOT "${KIT}" STREQUAL "Filtering" OR
         NOT "${TMP_FILENAME}" STREQUAL "vtkInformation")

        # add to the INPUT_FILES
        SET(INPUT_FILES ${INPUT_FILES} ${TMP_INPUT})

        # add the info to the init file
        SET(VTK_WRAPPER_INIT_DATA
          "${VTK_WRAPPER_INIT_DATA}${TMP_INPUT}\n")

      ENDIF(NOT "${KIT}" STREQUAL "Filtering" OR
            NOT "${TMP_FILENAME}" STREQUAL "vtkInformation")
    ENDIF (IGNORE_WRAP_EXCLUDE OR TMP_WRAP_SPECIAL OR NOT TMP_WRAP_EXCLUDE)
  ENDFOREACH(FILE)

  # finish the data file for the init file
  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${OUTPUT_DIR}/${TARGET}.data
    COPY_ONLY
    IMMEDIATE
    )

  # find out what kits this kit depends on
  SET(TMP_KIT_DEPENDS ${KIT_INTERFACE_LIBRARIES} ${KIT_LIBS})

  # search through the libs to find kits we depend on
  SET(OTHER_HIERARCHY_FILES)
  FOREACH (TMP_LIB ${TMP_KIT_DEPENDS})
    STRING(REGEX REPLACE "vtk(.*)" "\\1" TMP_KIT "${TMP_LIB}")
    STRING(TOUPPER "${TMP_KIT}" TMP_KIT_UPPER)
    FOREACH (TMP_KIT_CHECK ${VTK_KITS})
      STRING(REPLACE "_" "" TMP_KIT_CHECK_UPPER ${TMP_KIT_CHECK})
      IF("${TMP_KIT_UPPER}" STREQUAL "${TMP_KIT_CHECK_UPPER}")
        SET(OTHER_HIERARCHY_FILES ${OTHER_HIERARCHY_FILES}
          "${quote}${VTK_BINARY_DIR}/${TMP_KIT}/vtk${TMP_KIT}Hierarchy.txt${quote}")
      ENDIF("${TMP_KIT_UPPER}" STREQUAL "${TMP_KIT_CHECK_UPPER}")
    ENDFOREACH (TMP_KIT_CHECK ${VTK_KITS})
  ENDFOREACH (TMP_LIB ${TMP_KIT_DEPENDS})

  # build the hierarchy file when the kit library is built
  ADD_CUSTOM_COMMAND(
    TARGET vtk${KIT} PRE_BUILD
    COMMAND ${VTK_WRAP_HIERARCHY_EXE}
    ARGS
    "-o" "${quote}${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt${quote}"
    "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
    ${OTHER_HIERARCHY_FILES}
    COMMENT "Hierarchy Wrapping - generating vtk${KIT}Hierarchy.txt"
    ${verbatim}
    )

  # ugly but necessary
  ADD_DEPENDENCIES(vtk${KIT} vtkWrapHierarchy)

  # make a dummy target so that items that depend on the heirarchy
  # file will rebuild when the file changes, but will not have to
  # rebuild every time the kit library is built
  ADD_CUSTOM_COMMAND(
    OUTPUT ${OUTPUT_DIR}/${TARGET}.target
    ${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt
    DEPENDS vtk${KIT}

    COMMAND ${CMAKE_COMMAND}
    ARGS
    "-E" "touch" "${quote}${OUTPUT_DIR}/${TARGET}.target${quote}"
    ${verbatim}

    COMMAND ${VTK_WRAP_HIERARCHY_EXE}
    ARGS
    "-o" "${quote}${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt${quote}"
    "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
    ${OTHER_HIERARCHY_FILES}
    COMMENT "Hierarchy Wrapping - generating vtk${KIT}Hierarchy.txt"
    ${verbatim}
    )

ENDMACRO(VTK_WRAP_HIERARCHY)
