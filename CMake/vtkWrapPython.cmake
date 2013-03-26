#
# CMake implementation of the Wrap Python command.
#
macro(VTK_WRAP_PYTHON2 TARGET SOURCE_LIST_NAME)
  # convert to the WRAP3 signature
  vtk_wrap_python3(${TARGET} ${SOURCE_LIST_NAME} "${ARGN}")
endmacro()

macro(VTK_WRAP_PYTHON3 TARGET SRC_LIST_NAME SOURCES)
  if(NOT VTK_WRAP_PYTHON_INIT_EXE)
    if(TARGET vtkWrapPythonInit)
      set(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
    else()
      message(SEND_ERROR
        "VTK_WRAP_PYTHON_INIT_EXE not specified when calling VTK_WRAP_PYTHON3")
    endif()
  endif()
  if(NOT VTK_WRAP_PYTHON_EXE)
    if(TARGET vtkWrapPython)
      set(VTK_WRAP_PYTHON_EXE vtkWrapPython)
    else()
      message(SEND_ERROR
        "VTK_WRAP_PYTHON_EXE not specified when calling VTK_WRAP_PYTHON3")
    endif()
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

  # Initialize the custom target counter.
  if(VTK_WRAP_PYTHON_NEED_CUSTOM_TARGETS)
    set(VTK_WRAP_PYTHON_CUSTOM_COUNT "")
    set(VTK_WRAP_PYTHON_CUSTOM_NAME ${TARGET})
    set(VTK_WRAP_PYTHON_CUSTOM_LIST)
  endif()

  # start writing the input file for the init file
  set(VTK_WRAPPER_INIT_DATA "${TARGET}")

  # all the include directories
  if(VTK_WRAP_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  else()
    set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  endif()

  # collect the common wrapper-tool arguments
  set(_common_args)
  get_directory_property(_def_list DEFINITION COMPILE_DEFINITIONS)
  foreach(TMP_DEF ${_def_list})
    set(_common_args "${_common_args}-D${TMP_DEF}\n")
  endforeach()
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(_common_args "${_common_args}-I\"${INCLUDE_DIR}\"\n")
  endforeach()
  if(VTK_WRAP_HINTS)
    set(_common_args "${_common_args}--hints \"${VTK_WRAP_HINTS}\"\n")
  endif()
  if(KIT_HIERARCHY_FILE)
    set(_common_args "${_common_args}--types \"${KIT_HIERARCHY_FILE}\"\n")
  endif()

  # write wrapper-tool arguments to a file
  string(STRIP "${_common_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(_args_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.args)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${_args_file} @ONLY)

  # for each class
  foreach(FILE ${SOURCES})
    # should we wrap the file?
    get_source_file_property(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
    get_source_file_property(TMP_WRAP_SPECIAL ${FILE} WRAP_SPECIAL)

    # if we should wrap it
    if(TMP_WRAP_SPECIAL OR NOT TMP_WRAP_EXCLUDE)

      # what is the filename without the extension
      get_filename_component(TMP_FILENAME ${FILE} NAME_WE)

      # the input file might be full path so handle that
      get_filename_component(TMP_FILEPATH ${FILE} PATH)

      # compute the input filename
      if(TMP_FILEPATH)
        set(TMP_INPUT ${TMP_FILEPATH}/${TMP_FILENAME}.h)
      else()
        set(TMP_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TMP_FILENAME}.h)
      endif()

      # add the info to the init file
      set(VTK_WRAPPER_INIT_DATA
        "${VTK_WRAPPER_INIT_DATA}\n${TMP_FILENAME}")

      # new source file is namePython.cxx, add to resulting list
      set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}}
        ${TMP_FILENAME}Python.cxx)

      # add custom command to output
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx
        DEPENDS ${VTK_WRAP_PYTHON_EXE} ${VTK_WRAP_HINTS} ${TMP_INPUT} ${_args_file}
          ${KIT_HIERARCHY_FILE}
        COMMAND ${VTK_WRAP_PYTHON_EXE}
          ARGS
          "${quote}@${_args_file}${quote}"
          "-o" "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx${quote}"
          "${quote}${TMP_INPUT}${quote}"
        COMMENT "Python Wrapping - generating ${TMP_FILENAME}Python.cxx"
          ${verbatim}
        )

      # Add this output to a custom target if needed.
      if(VTK_WRAP_PYTHON_NEED_CUSTOM_TARGETS)
        set(VTK_WRAP_PYTHON_CUSTOM_LIST ${VTK_WRAP_PYTHON_CUSTOM_LIST}
          ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx)
        set(VTK_WRAP_PYTHON_CUSTOM_COUNT ${VTK_WRAP_PYTHON_CUSTOM_COUNT}x)
        if(VTK_WRAP_PYTHON_CUSTOM_COUNT MATCHES "^${VTK_WRAP_PYTHON_CUSTOM_LIMIT}$")
          set(VTK_WRAP_PYTHON_CUSTOM_NAME ${VTK_WRAP_PYTHON_CUSTOM_NAME}Hack)
          add_custom_target(${VTK_WRAP_PYTHON_CUSTOM_NAME}
            DEPENDS ${VTK_WRAP_PYTHON_CUSTOM_LIST})
          set(KIT_PYTHON_DEPS ${VTK_WRAP_PYTHON_CUSTOM_NAME})
          set(VTK_WRAP_PYTHON_CUSTOM_LIST)
          set(VTK_WRAP_PYTHON_CUSTOM_COUNT)
        endif()
      endif()
    endif()
  endforeach()

  # finish the data file for the init file
  configure_file(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COPY_ONLY
    IMMEDIATE
    )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    DEPENDS ${VTK_WRAP_PYTHON_INIT_EXE}
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COMMAND ${VTK_WRAP_PYTHON_INIT_EXE}
    ARGS
      "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data${quote}"
      "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx${quote}"
    COMMENT "Python Wrapping - generating ${TARGET}Init.cxx"
      ${verbatim}
    )

  # Create the Init File
  set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${TARGET}Init.cxx)

endmacro(VTK_WRAP_PYTHON3)

if(VTK_WRAP_PYTHON_FIND_LIBS)
  get_filename_component(_CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
  find_package(PythonLibs)

  # Use separate debug/optimized libraries if they are different.
  if(PYTHON_DEBUG_LIBRARY)
    if("${PYTHON_DEBUG_LIBRARY}" STREQUAL "${PYTHON_LIBRARY}")
      set(VTK_PYTHON_LIBRARIES ${PYTHON_LIBRARY})
    else()
      set(VTK_PYTHON_LIBRARIES
        optimized ${PYTHON_LIBRARY}
        debug ${PYTHON_DEBUG_LIBRARY})
    endif()
    set(VTK_WINDOWS_PYTHON_DEBUGGABLE 0)
    if(WIN32)
      if(PYTHON_DEBUG_LIBRARY MATCHES "_d")
        set(VTK_WINDOWS_PYTHON_DEBUGGABLE 1)
      endif()
    endif()
  else()
    set(VTK_PYTHON_LIBRARIES ${PYTHON_LIBRARY})
  endif()

  # Some python installations on UNIX need to link to extra libraries
  # such as zlib (-lz).  It is hard to automatically detect the needed
  # libraries so instead just give the user an easy way to specify
  # the libraries.  This should be needed only rarely.  It should
  # also be moved to the CMake FindPython.cmake module at some point.
  if(UNIX)
    if(NOT DEFINED PYTHON_EXTRA_LIBS)
      set(PYTHON_EXTRA_LIBS "" CACHE STRING
        "Extra libraries to link when linking to python (such as \"z\" for zlib).  Separate multiple libraries with semicolons.")
      mark_as_advanced(PYTHON_EXTRA_LIBS)
    endif()
  endif()

  # Include any extra libraries for python.
  set(VTK_PYTHON_LIBRARIES ${VTK_PYTHON_LIBRARIES} ${PYTHON_EXTRA_LIBS})
endif()

# Determine the location of the supplied header in the include_dirs supplied.
macro(vtk_find_header header include_dirs full_path)
  unset(${full_path})
  foreach(_dir ${include_dirs})
    if(EXISTS "${_dir}/${header}")
      set(${full_path} "${_dir}/${header}")
      break()
    endif()
  endforeach()
endmacro()

# Macro that just takes the name of the module, figure the rest out from there.
macro(vtk_wrap_python TARGET SRC_LIST_NAME module)
  if(NOT VTK_WRAP_PYTHON_INIT_EXE)
    if(TARGET vtkWrapPythonInit)
      set(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
    else()
      message(SEND_ERROR
        "VTK_WRAP_PYTHON_INIT_EXE not specified when calling VTK_WRAP_PYTHON3")
    endif()
  endif()
  if(NOT VTK_WRAP_PYTHON_EXE)
    if(TARGET vtkWrapPython)
      set(VTK_WRAP_PYTHON_EXE vtkWrapPython)
    else()
      message(SEND_ERROR
        "VTK_WRAP_PYTHON_EXE not specified when calling vtk_wrap_python")
    endif()
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

  # Initialize the custom target counter.
  if(VTK_WRAP_PYTHON_NEED_CUSTOM_TARGETS)
    set(VTK_WRAP_PYTHON_CUSTOM_COUNT "")
    set(VTK_WRAP_PYTHON_CUSTOM_NAME ${TARGET})
    set(VTK_WRAP_PYTHON_CUSTOM_LIST)
  endif()

  # start writing the input file for the init file
  set(VTK_WRAPPER_INIT_DATA "${TARGET}")

  # all the include directories
  if(${module}_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${${module}_INCLUDE_DIRS})
  elseif(VTK_WRAP_INCLUDE_DIRS)
    set(TMP_INCLUDE_DIRS ${VTK_WRAP_INCLUDE_DIRS})
  else()
    set(TMP_INCLUDE_DIRS ${VTK_INCLUDE_DIRS})
  endif()
  if(EXTRA_PYTHON_INCLUDE_DIRS)
    list(APPEND TMP_INCLUDE_DIRS ${EXTRA_PYTHON_INCLUDE_DIRS})
  endif()

  # collect the common wrapper-tool arguments
  set(_common_args)
  get_directory_property(_def_list DEFINITION COMPILE_DEFINITIONS)
  foreach(TMP_DEF ${_def_list})
    set(_common_args "${_common_args}-D${TMP_DEF}\n")
  endforeach()
  foreach(INCLUDE_DIR ${TMP_INCLUDE_DIRS})
    set(_common_args "${_common_args}-I\"${INCLUDE_DIR}\"\n")
  endforeach()
  if(VTK_WRAP_HINTS)
    set(_common_args "${_common_args}--hints \"${VTK_WRAP_HINTS}\"\n")
  endif()
  if(KIT_HIERARCHY_FILE)
    set(_common_args "${_common_args}--types \"${KIT_HIERARCHY_FILE}\"\n")
  endif()

  # write wrapper-tool arguments to a file
  string(STRIP "${_common_args}" CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(_args_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.args)
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${_args_file} @ONLY)

  # Decide what to do for each header.
  foreach(header ${${module}_HEADERS})
    # Everything in this block is for headers that will be wrapped.
    if(${module}_HEADER_${header}_WRAP_SPECIAL OR
       NOT ${module}_HEADER_${header}_WRAP_EXCLUDE)

      # Find the full path to the header file to be wrapped.
      vtk_find_header(${header}.h "${${module}_INCLUDE_DIRS}" class_header_path)
      if(NOT class_header_path)
        message(FATAL_ERROR "Could not find the ${header} header file.")
      endif()

      # add the info to the init file
      set(VTK_WRAPPER_INIT_DATA
        "${VTK_WRAPPER_INIT_DATA}\n${header}")

      # new source file is namePython.cxx, add to resulting list
      set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${header}Python.cxx)

      # add custom command to output
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${header}Python.cxx
        DEPENDS ${VTK_WRAP_PYTHON_EXE} ${VTK_WRAP_HINTS} ${class_header_path}
          ${_args_file} ${KIT_HIERARCHY_FILE}
        COMMAND ${VTK_WRAP_PYTHON_EXE}
          ARGS
          "${quote}@${_args_file}${quote}"
          "-o" "${quote}${CMAKE_CURRENT_BINARY_DIR}/${header}Python.cxx${quote}"
          "${quote}${class_header_path}${quote}"
        COMMENT "Python Wrapping - generating ${header}Python.cxx"
          ${verbatim}
        )

      # Add this output to a custom target if needed.
      if(VTK_WRAP_PYTHON_NEED_CUSTOM_TARGETS)
        set(VTK_WRAP_PYTHON_CUSTOM_LIST ${VTK_WRAP_PYTHON_CUSTOM_LIST}
          ${CMAKE_CURRENT_BINARY_DIR}/${header}Python.cxx)
        set(VTK_WRAP_PYTHON_CUSTOM_COUNT ${VTK_WRAP_PYTHON_CUSTOM_COUNT}x)
        if(VTK_WRAP_PYTHON_CUSTOM_COUNT MATCHES "^${VTK_WRAP_PYTHON_CUSTOM_LIMIT}$")
          set(VTK_WRAP_PYTHON_CUSTOM_NAME ${VTK_WRAP_PYTHON_CUSTOM_NAME}Hack)
          add_custom_target(${VTK_WRAP_PYTHON_CUSTOM_NAME}
            DEPENDS ${VTK_WRAP_PYTHON_CUSTOM_LIST})
          set(KIT_PYTHON_DEPS ${VTK_WRAP_PYTHON_CUSTOM_NAME})
          set(VTK_WRAP_PYTHON_CUSTOM_LIST)
          set(VTK_WRAP_PYTHON_CUSTOM_COUNT)
        endif()
      endif()
    else()
      message("${header} will not be wrapped.")
    endif()
  endforeach()

  # finish the data file for the init file
  configure_file(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COPY_ONLY
    IMMEDIATE
    )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
    DEPENDS ${VTK_WRAP_PYTHON_INIT_EXE}
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COMMAND ${VTK_WRAP_PYTHON_INIT_EXE}
    ARGS
      "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data${quote}"
      "${quote}${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx${quote}"
    COMMENT "Python Wrapping - generating ${TARGET}Init.cxx"
      ${verbatim}
    )

  # Create the Init File
  set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${TARGET}Init.cxx)

endmacro()
