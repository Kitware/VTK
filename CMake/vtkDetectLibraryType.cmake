include(CMakeParseArguments)
function (vtk_detect_library_type output)
  cmake_parse_arguments(vdlt
    ""
    "PATH"
    ""
    ${ARGN})

  if (NOT DEFINED vdlt_PATH)
    message(FATAL_ERROR
      "The `PATH` argument is required.")
  endif ()

  if (DEFINED vdlt_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_detect_library_type: "
      "${vdlt_UNPARSED_ARGUMENTS}")
  endif ()

  set(vdlt_type UNKNOWN)
  # Windows libraries all end with `.lib`. We need to detect the type based on
  # the contents of the library. However, MinGW does use different extensions.
  if (WIN32 AND NOT MINGW)
    # TODO: Implement by looking at the contents to see if it is a static or
    # shared library.
  else ()
    string(LENGTH "${vdlt_PATH}" vdlt_path_len)

    string(LENGTH "${CMAKE_SHARED_LIBRARY_SUFFIX}" vdlt_shared_suffix_len)
    math(EXPR vdlt_shared_idx "${vdlt_path_len} - ${vdlt_shared_suffix_len}")
    string(SUBSTRING "${vdlt_PATH}" "${vdlt_shared_idx}" -1 vdlt_shared_check)

    string(LENGTH "${CMAKE_STATIC_LIBRARY_SUFFIX}" vdlt_static_suffix_len)
    math(EXPR vdlt_static_idx "${vdlt_path_len} - ${vdlt_static_suffix_len}")
    string(SUBSTRING "${vdlt_PATH}" "${vdlt_static_idx}" -1 vdlt_static_check)

    if (vdlt_shared_check STREQUAL CMAKE_SHARED_LIBRARY_SUFFIX)
      set(vdlt_type SHARED)
    elseif (vdlt_static_check STREQUAL CMAKE_STATIC_LIBRARY_SUFFIX)
      set(vdlt_type STATIC)
    endif ()
  endif ()

  set("${output}"
    "${vdlt_type}"
    PARENT_SCOPE)
endfunction ()

function (vtk_detect_library_shared name target)
  if (VTK_MODULE_USE_EXTERNAL_${name})
    get_property(library_type
      TARGET    "${target}"
      PROPERTY  TYPE)
    if (library_type STREQUAL "SHARED_LIBRARY")
      set(is_shared 1)
    elseif (library_type STREQUAL "UNKNOWN_LIBRARY")
      option("VTK_MODULE_${name}_IS_SHARED" "Whether the ${name} in use is shared or not" ON)
      mark_as_advanced("VTK_MODULE_${name}_IS_SHARED")
      set(is_shared "${VTK_MODULE_${name}_IS_SHARED}")
    else ()
      set(is_shared 0)
    endif ()
  else ()
    set(is_shared "${BUILD_SHARED_LIBS}")
  endif ()

  set("${name}_is_shared"
    "${is_shared}"
    PARENT_SCOPE)
endfunction ()
