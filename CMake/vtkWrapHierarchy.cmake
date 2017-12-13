#
# a cmake macro to generate a text file with the class hierarchy
#
macro(VTK_WRAP_HIERARCHY module_name OUTPUT_DIR SOURCES)
  if(NOT VTK_WRAP_HIERARCHY_EXE)
    if(TARGET vtkWrapHierarchy)
      set(VTK_WRAP_HIERARCHY_EXE vtkWrapHierarchy)
    else()
      message(SEND_ERROR "VTK_WRAP_HIERARCHY_EXE not specified when calling VTK_WRAP_HIERARCHY")
    endif()
  endif()

  # collect the common wrapper-tool arguments
  if(NOT VTK_ENABLE_KITS)
    # write wrapper-tool arguments to a file
    set(_args_file ${module_name}Hierarchy.$<CONFIGURATION>.args)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_args_file} CONTENT "
$<$<BOOL:$<TARGET_PROPERTY:${module_name},COMPILE_DEFINITIONS>>:
-D\"$<JOIN:$<TARGET_PROPERTY:${module_name},COMPILE_DEFINITIONS>,\"
-D\">\">
$<$<BOOL:$<TARGET_PROPERTY:${module_name},INCLUDE_DIRECTORIES>>:
-I\"$<JOIN:$<TARGET_PROPERTY:${module_name},INCLUDE_DIRECTORIES>,\"
-I\">\">
")
  else()
    set(_common_args)
    get_directory_property(_def_list DEFINITION COMPILE_DEFINITIONS)
    foreach(TMP_DEF ${_def_list})
      set(_common_args "${_common_args}-D${TMP_DEF}\n")
    endforeach()

    # all the include directories
    if(VTK_WRAP_INCLUDE_DIRS)
      set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
    else()
      set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
    endif()
    foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
      set(_common_args "${_common_args}-I\"${INCLUDE_DIR}\"\n")
    endforeach()

    # write wrapper-tool arguments to a file
    set(_args_file ${module_name}Hierarchy.args)
    string(STRIP "${_common_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
    configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
      ${_args_file} @ONLY)
  endif()

  # list of all files to wrap
  set(VTK_WRAPPER_INIT_DATA)
  # list of used files
  set(INPUT_FILES)

  # For each class
  foreach(FILE ${SOURCES})

    # what is the filename without the extension
    get_filename_component(TMP_FILENAME ${FILE} NAME_WE)

    # get the absolute path to the file
    get_filename_component(TMP_FULLPATH ${FILE} ABSOLUTE)

    # get the directory
    get_filename_component(TMP_FILEPATH ${TMP_FULLPATH} PATH)

    # assume header file is in the same directory
    set(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h)

    # include all non-private headers in the hierarchy files
    set(TMP_EXCLUDE_FROM_HIERARCHY OFF)
    get_source_file_property(TMP_SKIP ${FILE} SKIP_HEADER_INSTALL)
    if(TMP_SKIP)
      set(TMP_EXCLUDE_FROM_HIERARCHY ON)
    endif()

    # ensure that header exists
    if(NOT EXISTS ${TMP_INPUT})
      set(TMP_EXCLUDE_FROM_HIERARCHY ON)
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
        "${VTK_WRAPPER_INIT_DATA}${TMP_INPUT};${module_name}")

      set(VTK_WRAPPER_INIT_DATA "${VTK_WRAPPER_INIT_DATA}\n")

    endif()
  endforeach()

  # finish the data file for the init file
  configure_file(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${module_name}Hierarchy.data
    @ONLY
    )

  # search through the deps to find modules we depend on
  set(OTHER_HIERARCHY_FILES)
  set(OTHER_HIERARCHY_TARGETS)
  # Don't use ${module_name}_DEPENDS. That list also includes COMPILE_DEPENDS,
  # which aren't library dependencies, merely dependencies for generators and
  # such. Instead, use _WRAP_DEPENDS which includes the DEPENDS and the
  # PRIVATE_DEPENDS from module.cmake, but not COMPILE_DEPENDS.
  foreach(dep ${${module_name}_WRAP_DEPENDS})
    if(NOT "${module_name}" STREQUAL "${dep}")
      if(NOT ${dep}_EXCLUDE_FROM_WRAPPING)
        list(APPEND OTHER_HIERARCHY_FILES "${${dep}_WRAP_HIERARCHY_FILE}")
        if (TARGET "${dep}Hierarchy")
          list(APPEND OTHER_HIERARCHY_TARGETS "${dep}Hierarchy")
        endif ()
      endif()
    endif()
  endforeach()

  # write wrapper-tool arguments to a file
  set(_other_hierarchy_args )
  foreach(hierarchy_file ${OTHER_HIERARCHY_FILES})
    set(_other_hierarchy_args "${_other_hierarchy_args}\"${hierarchy_file}\"\n")
  endforeach()
  set(_other_hierarchy_args_file ${module_name}OtherHierarchyFiles.args)
  string(STRIP "${_other_hierarchy_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
    ${_other_hierarchy_args_file} @ONLY)

  if (CMAKE_GENERATOR MATCHES "Ninja")
    set(hierarchy_depends ${OTHER_HIERARCHY_FILES})
  else ()
    set(hierarchy_depends ${OTHER_HIERARCHY_TARGETS})
  endif ()

  add_custom_command(
    OUTPUT  "${OUTPUT_DIR}/${module_name}Hierarchy.txt"
    COMMAND ${VTK_WRAP_HIERARCHY_EXE}
            @${_args_file} -o ${OUTPUT_DIR}/${module_name}Hierarchy.txt
            ${module_name}Hierarchy.data
            @${_other_hierarchy_args_file}
    COMMENT "For ${module_name} - updating ${module_name}Hierarchy.txt"
    DEPENDS ${VTK_WRAP_HIERARCHY_EXE}
            ${CMAKE_CURRENT_BINARY_DIR}/${_args_file}
            ${CMAKE_CURRENT_BINARY_DIR}/${module_name}Hierarchy.data
            ${hierarchy_depends}
            ${INPUT_FILES}
    VERBATIM
    )
  add_custom_target(${module_name}Hierarchy
    DEPENDS
      ${OUTPUT_DIR}/${module_name}Hierarchy.txt)

endmacro()
