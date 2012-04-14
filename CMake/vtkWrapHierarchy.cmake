#
# a cmake macro to generate a text file with the class hierarchy
#
macro(VTK_WRAP_HIERARCHY TARGET OUTPUT_DIR SOURCES)
  if(NOT VTK_WRAP_HIERARCHY_EXE)
    message(SEND_ERROR "VTK_WRAP_HIERARCHY_EXE not specified when calling VTK_WRAP_HIERARCHY")
  endif()

  # The shell into which nmake.exe executes the custom command has some issues
  # with mixing quoted and unquoted arguments :( Let's help.
  if(CMAKE_GENERATOR MATCHES "NMake Makefiles")
    set(verbatim "")
    set(quote "\"")
  else()
    set(verbatim "VERBATIM")
    set(quote "")
  endif()

  # all the compiler "-D" args
  get_directory_property(TMP_DEF_LIST
    DIRECTORY ${${vtk-module}_SOURCE_DIR}
    DEFINITION COMPILE_DEFINITIONS)
  set(TMP_DEFINITIONS)
  foreach(TMP_DEF ${TMP_DEF_LIST})
    set(TMP_DEFINITIONS ${TMP_DEFINITIONS} -D "${quote}${TMP_DEF}${quote}")
  endforeach()

  # all the include directories
  if(VTK_WRAP_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  else()
    set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  endif()
  set(TMP_INCLUDE)
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(TMP_INCLUDE ${TMP_INCLUDE} -I "${quote}${INCLUDE_DIR}${quote}")
  endforeach()

  # list of all files to wrap
  set(VTK_WRAPPER_INIT_DATA)
  # list of used files
  set(INPUT_FILES)

  # For each class
  foreach(FILE ${SOURCES})

    # file properties to include in the hierarchy file
    get_property(TMP_WRAP_EXCLUDE SOURCE ${FILE} PROPERTY WRAP_EXCLUDE)
    get_source_file_property(TMP_WRAP_SPECIAL ${FILE} WRAP_SPECIAL)
    get_source_file_property(TMP_ABSTRACT ${FILE} ABSTRACT)

    # what is the filename without the extension
    get_filename_component(TMP_FILENAME ${FILE} NAME_WE)

    # get the absolute path to the file
    get_filename_component(TMP_FULLPATH ${FILE} ABSOLUTE)

    # get the directory
    get_filename_component(TMP_FILEPATH ${TMP_FULLPATH} PATH)

    # assume header file is in the same directory
    set(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h)

    # default to including all available headers in the hierarchy files
    set(TMP_EXCLUDE_FROM_HIERARCHY OFF)

    # ensure that header exists (assume it exists if it is marked as wrapped)
    if(TMP_WRAP_EXCLUDE AND NOT TMP_WRAP_SPECIAL)
      if(NOT EXISTS ${TMP_INPUT})
        set(TMP_EXCLUDE_FROM_HIERARCHY ON)
      endif()
    endif()

    # Exclude this huge generated header file
    if("${TMP_FILENAME}" STREQUAL "vtkgl")
      set(TMP_EXCLUDE_FROM_HIERARCHY ON)
    endif()

    if(NOT TMP_EXCLUDE_FROM_HIERARCHY)
      # add to the INPUT_FILES
      list(APPEND INPUT_FILES ${TMP_INPUT})

      # add the info to the init file
      set(VTK_WRAPPER_INIT_DATA
        "${VTK_WRAPPER_INIT_DATA}${TMP_INPUT};${vtk-module}")

      if(TMP_ABSTRACT)
        set(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};ABSTRACT")
      endif()

      if(TMP_WRAP_EXCLUDE)
        set(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};WRAP_EXCLUDE")
      endif()

      if(TMP_WRAP_SPECIAL)
        set(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA};WRAP_SPECIAL")
      endif()

      set(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA}\n")

    endif()
  endforeach()

  # finish the data file for the init file
  configure_file(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${OUTPUT_DIR}/${TARGET}.data
    COPY_ONLY
    IMMEDIATE
    )

  # search through the deps to find modules we depend on
  set(OTHER_HIERARCHY_FILES)
  set(OTHER_HIERARCHY_TARGETS)
  foreach(dep ${VTK_MODULE_${vtk-module}_DEPENDS})
    if(NOT "${vtk-module}" STREQUAL "${dep}")
      if(NOT VTK_MODULE_${dep}_EXCLUDE_FROM_WRAPPING)
        list(APPEND OTHER_HIERARCHY_FILES
		"${quote}${${dep}_BINARY_DIR}/${dep}Hierarchy.txt${quote}")
        list(APPEND OTHER_HIERARCHY_TARGETS ${dep})
      endif()
    endif()
  endforeach()

  if(NOT CMAKE_GENERATOR MATCHES "Visual Studio.*")
    # Build the hierarchy file when the module library is built, this
    # ensures that the file is built when modules in other libraries
    # need it (i.e. they depend on this module's library, but if this
    # module's library is built, then the hierarchy file is also built).
    add_custom_command(
      TARGET ${vtk-module} POST_BUILD
      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
        ${TMP_DEFINITIONS}
        ${TMP_INCLUDE}
        "-o" "${quote}${OUTPUT_DIR}/${vtk-module}Hierarchy.txt${quote}"
        "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
        ${OTHER_HIERARCHY_FILES}

      COMMAND ${CMAKE_COMMAND}
        "-E" "touch" "${quote}${OUTPUT_DIR}/${TARGET}.target${quote}"

      COMMENT "For ${vtk-module} - updating ${vtk-module}Hierarchy.txt"
        ${verbatim}
      )

    # Force the above custom command to execute if hierarchy tool changes
    add_dependencies(${vtk-module} vtkWrapHierarchy)

    # Add a custom-command for when the hierarchy file is needed
    # within its own module.  A dummy target is needed because the
    # vtkWrapHierarchy tool only changes the timestamp on the
    # hierarchy file if the VTK hierarchy actually changes.
    add_custom_command(
      OUTPUT ${OUTPUT_DIR}/${TARGET}.target
      ${OUTPUT_DIR}/${vtk-module}Hierarchy.txt
      DEPENDS ${VTK_WRAP_HIERARCHY_EXE}
      ${OUTPUT_DIR}/${TARGET}.data ${INPUT_FILES}

      COMMAND ${CMAKE_COMMAND}
      "-E" "touch" "${quote}${OUTPUT_DIR}/${TARGET}.target${quote}"

      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
      ${TMP_DEFINITIONS}
      ${TMP_INCLUDE}
      "-o" "${quote}${OUTPUT_DIR}/${vtk-module}Hierarchy.txt${quote}"
      "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
      ${OTHER_HIERARCHY_FILES}

      COMMENT "Hierarchy Wrapping - updating ${vtk-module}Hierarchy.txt"
      ${verbatim}
      )
  else()
    # On Visual Studio builds, the target-timestamp trick does not work,
    # so only build hierarchy files when library is built.
    add_custom_command(
      TARGET ${vtk-module} POST_BUILD

      COMMAND ${VTK_WRAP_HIERARCHY_EXE}
      ${TMP_DEFINITIONS}
      ${TMP_INCLUDE}
      "-o" "${quote}${OUTPUT_DIR}/${vtk-module}Hierarchy.txt${quote}"
      "${quote}${OUTPUT_DIR}/${TARGET}.data${quote}"
      ${OTHER_HIERARCHY_FILES}

      COMMENT "Updating ${vtk-module}Hierarchy.txt"
      ${verbatim}
      )
    # Set target-level dependencies so that everything builds in the
    # correct order, particularly the libraries.
    add_dependencies(${vtk-module} vtkWrapHierarchy ${OTHER_HIERARCHY_TARGETS})
  endif()

endmacro()
