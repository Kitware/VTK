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

  # all the compiler "-D" args
  GET_DIRECTORY_PROPERTY(TMP_DEF_LIST DEFINITION COMPILE_DEFINITIONS)
  SET(TMP_DEFINITIONS)
  FOREACH(TMP_DEF ${TMP_DEF_LIST})
    SET(TMP_DEFINITIONS ${TMP_DEFINITIONS} -D "${quote}${TMP_DEF}${quote}")
  ENDFOREACH(TMP_DEF ${TMP_DEF_LIST})

  # all the include directories
  IF(VTK_WRAP_INCLUDE_DIRS)
    SET(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  ELSE(VTK_WRAP_INCLUDE_DIRS)
    SET(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  ENDIF(VTK_WRAP_INCLUDE_DIRS)
  SET(TMP_INCLUDE)
  FOREACH(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    SET(TMP_INCLUDE ${TMP_INCLUDE} -I "${quote}${INCLUDE_DIR}${quote}")
  ENDFOREACH(INCLUDE_DIR ${TMP_INCLUDE_DIRS})

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
    GET_SOURCE_FILE_PROPERTY(TMP_ABSTRACT ${FILE} ABSTRACT)

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

      IF(NOT "${KIT}" STREQUAL "Rendering" OR
         (NOT "${TMP_FILENAME}" STREQUAL "vtkgl" AND
          NOT "${TMP_FILENAME}" STREQUAL "vtkOpenGLState"))
      IF(NOT "${KIT}" STREQUAL "Filtering" OR
         NOT "${TMP_FILENAME}" STREQUAL "vtkInformation")

        # add to the INPUT_FILES
        SET(INPUT_FILES ${INPUT_FILES} ${TMP_INPUT})

        # add the info to the init file
        SET(VTK_WRAPPER_INIT_DATA
          "${VTK_WRAPPER_INIT_DATA}${TMP_INPUT};vtk${KIT}")

        IF (TMP_ABSTRACT)
          SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};ABSTRACT")
        ENDIF (TMP_ABSTRACT)

        IF (TMP_WRAP_EXCLUDE)
          SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};WRAP_EXCLUDE")
        ENDIF (TMP_WRAP_EXCLUDE)

        IF (TMP_WRAP_SPECIAL)
          SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};WRAP_SPECIAL")
        ENDIF (TMP_WRAP_SPECIAL)

        SET(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA}\n")

      ENDIF(NOT "${KIT}" STREQUAL "Filtering" OR
            NOT "${TMP_FILENAME}" STREQUAL "vtkInformation")
      ENDIF(NOT "${KIT}" STREQUAL "Rendering" OR
            (NOT "${TMP_FILENAME}" STREQUAL "vtkgl" AND
             NOT "${TMP_FILENAME}" STREQUAL "vtkOpenGLState"))
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
  SET(OTHER_HIERARCHY_TARGETS)
  FOREACH (TMP_LIB ${TMP_KIT_DEPENDS})
    STRING(REGEX REPLACE "vtk(.*)" "\\1" TMP_KIT "${TMP_LIB}")
    IF(NOT "${TMP_KIT}" STREQUAL "${KIT}")
      STRING(TOUPPER "${TMP_KIT}" TMP_KIT_UPPER)
      FOREACH (TMP_KIT_CHECK ${VTK_KITS})
        STRING(REPLACE "_" "" TMP_KIT_CHECK_UPPER ${TMP_KIT_CHECK})
        IF("${TMP_KIT_UPPER}" STREQUAL "${TMP_KIT_CHECK_UPPER}")
          SET(OTHER_HIERARCHY_FILES ${OTHER_HIERARCHY_FILES}
            "${quote}${VTK_BINARY_DIR}/${TMP_KIT}/vtk${TMP_KIT}Hierarchy.txt${quote}")
          SET(OTHER_HIERARCHY_TARGETS ${OTHER_HIERARCHY_TARGETS}
            vtk${TMP_KIT})
        ENDIF("${TMP_KIT_UPPER}" STREQUAL "${TMP_KIT_CHECK_UPPER}")
      ENDFOREACH (TMP_KIT_CHECK ${VTK_KITS})
    ENDIF(NOT "${TMP_KIT}" STREQUAL "${KIT}")
  ENDFOREACH (TMP_LIB ${TMP_KIT_DEPENDS})

  IF(NOT CMAKE_GENERATOR MATCHES "Visual Studio.*")
    # Build the hierarchy file when the kit library is built, this
    # ensures that the file is built when kits in other libraries
    # need it (i.e. they depend on this kits library, but if this
    # kits library is built, then the hierarchy file is also built).
    ADD_CUSTOM_COMMAND(
      TARGET vtk${KIT} POST_BUILD

      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
      ${TMP_DEFINITIONS}
      ${TMP_INCLUDE}
      "-o" "${quote}${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt${quote}"
      "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
      ${OTHER_HIERARCHY_FILES}

      COMMAND ${CMAKE_COMMAND}
      "-E" "touch" "${quote}${OUTPUT_DIR}/${TARGET}.target${quote}"

      COMMENT "For vtk${KIT} - updating vtk${KIT}Hierarchy.txt"
      ${verbatim}
      )

    # Force the above custom command to execute if hierarchy tool changes
    ADD_DEPENDENCIES(vtk${KIT} vtkWrapHierarchy)

    # Add a custom-command for when the hierarchy file is needed
    # within its own kit.  A dummy target is needed because the
    # vtkWrapHierarchy tool only changes the timestamp on the
    # hierarchy file if the VTK hierarchy actually changes.
    ADD_CUSTOM_COMMAND(
      OUTPUT ${OUTPUT_DIR}/${TARGET}.target
      ${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt
      DEPENDS ${VTK_WRAP_HIERARCHY_EXE}
      ${OUTPUT_DIR}/${TARGET}.data ${INPUT_FILES}

      COMMAND ${CMAKE_COMMAND}
      "-E" "touch" "${quote}${OUTPUT_DIR}/${TARGET}.target${quote}"

      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
      ${TMP_DEFINITIONS}
      ${TMP_INCLUDE}
      "-o" "${quote}${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt${quote}"
      "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
      ${OTHER_HIERARCHY_FILES}

      COMMENT "Hierarchy Wrapping - updating vtk${KIT}Hierarchy.txt"
      ${verbatim}
      )
  ELSE(NOT CMAKE_GENERATOR MATCHES "Visual Studio.*")
    # On Visual Studio builds, the target-timestamp trick does not work,
    # so only build hierarchy files when library is built.
    ADD_CUSTOM_COMMAND(
      TARGET vtk${KIT} POST_BUILD

      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
      ${TMP_DEFINITIONS}
      ${TMP_INCLUDE}
      "-o" "${quote}${OUTPUT_DIR}/vtk${KIT}Hierarchy.txt${quote}"
      "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
      ${OTHER_HIERARCHY_FILES}

      COMMENT "Updating vtk${KIT}Hierarchy.txt"
      ${verbatim}
      )
    # Set target-level dependencies so that everything builds in the
    # correct order, particularly the libraries.
    ADD_DEPENDENCIES(vtk${KIT} vtkWrapHierarchy ${OTHER_HIERARCHY_TARGETS})
  ENDIF(NOT CMAKE_GENERATOR MATCHES "Visual Studio.*")

ENDMACRO(VTK_WRAP_HIERARCHY)
