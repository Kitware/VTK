#[==[
@brief Detect library type of a library

Sometimes it needs to be known whether a library is shared or static on a
system in order to change the usage requirements of an imported target
representing that library. This commonly occurs between static and shared
builds that share a set of installed headers. This function returns one of
`SHARED`, `STATIC`, or `UNKNOWN` into the variable passed as the first
argument.

~~~
vtk_detect_library_type(<variable>
  PATH <path>)
~~~
#]==]
function (vtk_detect_library_type output)
  cmake_parse_arguments(PARSE_ARGV 1 vdlt
    ""
    "PATH"
    "")

  if (NOT DEFINED vdlt_PATH)
    message(FATAL_ERROR
      "The `PATH` argument is required.")
  endif ()

  if (DEFINED vdlt_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_detect_library_type: "
      "${vdlt_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT vdlt_PATH)
    message(FATAL_ERROR
      "The `PATH` argument is empty.")
  endif ()

  set(vdlt_type UNKNOWN)
  # Windows libraries all end with `.lib`. We need to detect the type based on
  # the contents of the library. However, MinGW does use different extensions.
  if (WIN32 AND NOT MINGW)
    find_program(DUMPBIN_EXECUTABLE
      NAMES dumpbin
      DOC   "Path to the dumpbin executable")
    mark_as_advanced(DUMPBIN_EXECUTABLE)
    execute_process(
      COMMAND "${DUMPBIN_EXECUTABLE}"
              /HEADERS
              "${vdlt_PATH}"
      OUTPUT_VARIABLE vdlt_out
      ERROR_VARIABLE  vdlt_err
      RESULT_VARIABLE vdlt_res)
    if (vdlt_res)
      message(WARNING
        "Failed to run `dumpbin` on ${vdlt_PATH}. Cannot determine "
        "shared/static library type: ${vdlt_err}")
    else ()
      if (vdlt_out MATCHES "DLL name     :")
        set(vdlt_type SHARED)
      else ()
        set(vdlt_type STATIC)
      endif ()
    endif ()
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

    # when import suffix != static suffix, we can disambiguate static and import
    if (WIN32 AND NOT CMAKE_IMPORT_LIBRARY_SUFFIX STREQUAL CMAKE_STATIC_LIBRARY_SUFFIX)
      string(LENGTH "${CMAKE_IMPORT_LIBRARY_SUFFIX}" vdlt_import_suffix_len)
      math(EXPR vdlt_import_idx "${vdlt_path_len} - ${vdlt_import_suffix_len}")
      string(SUBSTRING "${vdlt_PATH}" "${vdlt_import_idx}" -1 vdlt_import_check)
      if (vdlt_import_check STREQUAL CMAKE_IMPORT_LIBRARY_SUFFIX)
        set(vdlt_type SHARED)
      endif ()
    endif ()
  endif ()

  set("${output}"
    "${vdlt_type}"
    PARENT_SCOPE)
endfunction ()

#[==[
@brief Detect whether an imported target is shared or not

This is intended for use with modules using
[vtk_module_third_party_external](https://docs.vtk.org/en/latest/api/cmake/vtkModule.html#command:vtk_module_third_party_external)
to detect whether that module is shared or
not. Generally, this should be replaced with the `Find` module providing this
information and modifying the usage requirements as necessary instead, but it
is not always possible.

~~~
vtk_detect_library_shared(<name> <target>)
~~~

Sets `<name>_is_shared` in the caller's scope if `<target>` is a shared
library. If it is an `UNKNOWN_LIBRARY`, a cache variable is exposed to allow
the user to provide the information if it ends up breaking something.
#]==]
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
