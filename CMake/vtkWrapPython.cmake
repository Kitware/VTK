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
        "VTK_WRAP_PYTHON_INIT_EXE not specified when calling vtk_wrap_python")
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

  # Initialize the custom target counter.
  if(VTK_WRAP_PYTHON_NEED_CUSTOM_TARGETS)
    set(VTK_WRAP_PYTHON_CUSTOM_COUNT "")
    set(VTK_WRAP_PYTHON_CUSTOM_NAME ${TARGET})
    set(VTK_WRAP_PYTHON_CUSTOM_LIST)
  endif()

  # start writing the input file for the init file
  set(VTK_WRAPPER_INIT_DATA "${TARGET}")

  # collect the common wrapper-tool arguments
  set(_common_args)
  foreach(file IN LISTS VTK_WRAP_HINTS)
    set(_common_args "${_common_args}--hints \"${file}\"\n")
  endforeach()
  foreach(file IN LISTS KIT_HIERARCHY_FILE)
    set(_common_args "${_common_args}--types \"${file}\"\n")
  endforeach()

  # write wrapper-tool arguments to a file
  set(_args_file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.$<CONFIGURATION>.args)
  file(GENERATE OUTPUT ${_args_file} CONTENT "${_common_args}
$<$<BOOL:$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>:
-D\"$<JOIN:$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>,\"
-D\">\">
$<$<BOOL:$<TARGET_PROPERTY:${TARGET},INCLUDE_DIRECTORIES>>:
-I\"$<JOIN:$<TARGET_PROPERTY:${TARGET},INCLUDE_DIRECTORIES>,\"
-I\">\">
")

  if (CMAKE_GENERATOR MATCHES "Ninja")
    set(hierarchy_depend ${KIT_HIERARCHY_FILE})
  else ()
    string(LENGTH "${TARGET}" target_length)
    math(EXPR target_length "${target_length} - 6")
    string(SUBSTRING "${TARGET}" 0 "${target_length}" target_basename)
    get_property(is_kit GLOBAL
      PROPERTY "_vtk_${target_basename}_is_kit")
    if (is_kit)
      set(hierarchy_depend)
      get_property(kit_modules GLOBAL
        PROPERTY "_vtk_${target_basename}_kit_modules")
      message("depends for ${TARGET}: ${kit_modules}")
      foreach (depend IN LISTS kit_modules)
        if(TARGET "${depend}Hierarchy")
          list(APPEND hierarchy_depend
            "${depend}Hierarchy")
        endif()
      endforeach ()
    else ()
      set(hierarchy_depend "${target_basename}Hierarchy")
    endif ()
  endif ()

  # for each class
  foreach(FILE ${SOURCES})
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
    list(APPEND ${SRC_LIST_NAME} ${TMP_FILENAME}Python.cxx)

    # add custom command to output
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx
      DEPENDS ${VTK_WRAP_PYTHON_EXE}
              ${VTK_WRAP_HINTS}
              ${TMP_INPUT}
              ${_args_file}
              ${hierarchy_depend}
      COMMAND ${VTK_WRAP_PYTHON_EXE}
              @${_args_file}
              -o ${CMAKE_CURRENT_BINARY_DIR}/${TMP_FILENAME}Python.cxx
              ${TMP_INPUT}
      COMMENT "Python Wrapping - generating ${TMP_FILENAME}Python.cxx"
      VERBATIM
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
  endforeach()

  # finish the data file for the init file
  configure_file(
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    @ONLY
    )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
           ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}InitImpl.cxx
    DEPENDS ${VTK_WRAP_PYTHON_INIT_EXE}
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
    COMMAND ${VTK_WRAP_PYTHON_INIT_EXE}
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.data
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Init.cxx
            ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}InitImpl.cxx
    COMMENT "Python Wrapping - generating ${TARGET}Init.cxx"
    VERBATIM
    )

  # Create the Init File
  set(${SRC_LIST_NAME} ${${SRC_LIST_NAME}} ${TARGET}InitImpl.cxx)

endmacro()

if(VTK_WRAP_PYTHON_FIND_LIBS)
  get_filename_component(_CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
  if (VTK_UNDEFINED_SYMBOLS_ALLOWED)
    set(_QUIET_LIBRARY "QUIET")
  else()
    set(_QUIET_LIBRARY "REQUIRED")
  endif()
  find_package(PythonLibs ${_QUIET_LIBRARY})

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

# Macro that just takes the a list of module names, figure the rest out from there.
macro(vtk_wrap_python TARGET SRC_LIST_NAME)
  # List of all headers to wrap.
  set(headers_to_wrap)

  foreach(module ${ARGN})

  # Decide what to do for each header.
  foreach(header ${${module}_HEADERS})
    # Find the full path to the header file to be wrapped.
    vtk_find_header(${header}.h "${${module}_INCLUDE_DIRS}" class_header_path)
    if(NOT class_header_path)
      message(FATAL_ERROR "Could not find ${header}.h for Python wrapping!")
    endif()

    # The new list of headers has the full path to each file.
    list(APPEND headers_to_wrap ${class_header_path})
  endforeach()

  endforeach() # end ARGN loop

  # Delegate to vtk_wrap_python3
  vtk_wrap_python3(${TARGET} ${SRC_LIST_NAME} "${headers_to_wrap}" ${ARGN})
endmacro()
