#[==[.md
# `vtkModuleTesting`

VTK uses the [ExternalData][] CMake module to handle the data management for
its test suite. Test data is only downloaded when a test which requires it is
enabled and it is cached so that every build does not need to redownload the
same data.

To facilitate this workflow, there are a number of CMake functions available in
order to indicate that test data is required.

[ExternalData]: TODO
#]==]

include(ExternalData)
get_filename_component(_vtkModuleTesting_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

#[==[.md
## Loading data

Data may be downloaded manually using this function:

~~~
vtk_module_test_data(<PATHSPEC>...)
~~~

This will download data inside of the input data directory for the modules
being built at that time (see the `TEST_INPUT_DATA_DIRECTORY` argument of
`vtk_module_build`).

For supported `PATHSPEC` syntax, see the
[associated documentation][ExternalData pathspecs] in `ExternalData`. These
arguments are already wrapped in the `DATA{}` syntax and are assumed to be
relative paths from the input data directory.

[ExternalData pathspecs]: TODO
#]==]
function (vtk_module_test_data)
  set(data_args)
  foreach (arg IN LISTS ARGN)
    if (IS_ABSOLUTE "${arg}")
      list(APPEND data_args
        "DATA{${arg}}")
    else ()
      list(APPEND data_args
        "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/${arg}}")
    endif ()
  endforeach ()

  ExternalData_Expand_Arguments("${_vtk_build_TEST_DATA_TARGET}" _ ${data_args})
endfunction ()

#[==[.md
## Creating test executables

This function creates an executable from the list of sources passed to it. It
is automatically linked to the module the tests are intended for as well as any
declared test dependencies of the module.

~~~
vtk_module_test_executable(<NAME> <SOURCE>...)
~~~

This function is not usually used directly, but instead through the other
convenience functions.
#]==]
function (vtk_module_test_executable name)
  add_executable("${name}" ${ARGN})
  get_property(test_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_test}_test_depends")
  get_property(test_optional_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_test}_test_optional_depends")
  set(optional_depends_flags)
  foreach (test_optional_depend IN LISTS test_optional_depends)
    if (TARGET "${test_optional_depend}")
      list(APPEND test_depends
        "${test_optional_depend}")
      set(test_optional_depend_flag "1")
    else ()
      set(test_optional_depend_flag "0")
    endif ()
    string(REPLACE "::" "_" safe_test_optional_depend "${test_optional_depend}")
    list(APPEND optional_depends_flags
      "VTK_MODULE_ENABLE_${safe_test_optional_depend}=${test_optional_depend_flag}")
  endforeach ()

  target_link_libraries("${name}"
    PRIVATE
      "${_vtk_build_test}"
      ${test_depends})
  target_compile_definitions("${name}"
    PRIVATE
      ${optional_depends_flags})

  vtk_module_autoinit(
    TARGETS "${name}"
    MODULES "${_vtk_build_test}"
            ${test_depends})
endfunction ()

#[==[.md
## Test name parsing

Test names default to using the basename of the filename which contains the
test. Two tests may share the same file by prefixing with a custom name for the
test and a comma.

The two parsed syntaxes are:

  - `CustomTestName,TestFile`
  - `TestFile`

Note that `TestFile` should already have had its extension stripped (usually
done by `_vtk_test_parse_args`).

In general, the name of a test will be `<EXENAME>-<TESTNAME>`, however, by
setting `vtk_test_prefix`, the test name will instead be
`<EXENAME>-<PREFIX><TESTNAME>`.
#]==]

#[==[.md INTERNAL
This function parses the name from a testspec. The calling scope has
`test_name` and `test_file` variables set in it.

~~~
_vtk_test_parse_name(<TESTSPEC>)
~~~
#]==]
function (_vtk_test_parse_name name)
  if (name AND name MATCHES "^([^,]*),(.*)$")
    set(test_name "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(test_file "${CMAKE_MATCH_2}" PARENT_SCOPE)
  else ()
    set(test_name "${name}" PARENT_SCOPE)
    set(test_file "${name}" PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.md
## Test function arguments

Each test is specified  using one of the two following syntaxes

  - `<NAME>.<SOURCE_EXT>`
  - `<NAME>.<SOURCE_EXT>,<OPTIONS>`

Where `NAME` is a valid test name. If present, the specified `OPTIONS` are only
for the associated test. The expected extension is specified by the associated
test function.
#]==]

#[==[.md INTERNAL
Given a list of valid "options", this function will parse out a the following
variables:

  - `args`: Unrecognized arguments. These should be interpreted as arguments
    that should be passed on the command line to all tests in this parse group.
  - `options`: Options specified globally (for all tests in this group).
  - `names`: A list containing all named tests. These should be parsed by
    `_vtk_test_parse_name`.
  - `_<NAME>_options`: Options specific to a certain test.

~~~
_vtk_test_parse_args(<OPTIONS> <SOURCE_EXT> <ARG>...)
~~~

In order to be recognized as a source file, the `SOURCE_EXT` must be used.
Without it, all non-option arguments are placed into `args`. Each test is
parsed out matching these:
#]==]
function (_vtk_test_parse_args options source_ext)
  set(global_options)
  set(names)
  set(args)

  foreach (arg IN LISTS ARGN)
    set(handled 0)
    foreach (option IN LISTS options)
      if (arg STREQUAL option)
        list(APPEND global_options "${option}")
        set(handled 1)
        break ()
      endif ()
    endforeach ()
    if (handled)
      # Do nothing.
    elseif (source_ext AND arg MATCHES "^([^.]*)\\.${source_ext},?(.*)$")
      set(name "${CMAKE_MATCH_1}")
      string(REPLACE "," ";" "_${name}_options" "${CMAKE_MATCH_2}")
      list(APPEND names "${name}")
    else ()
      list(APPEND args "${arg}")
    endif ()
  endforeach ()

  foreach (name IN LISTS names)
    set("_${name}_options" "${_${name}_options}"
      PARENT_SCOPE)
  endforeach ()
  set(options "${global_options}"
    PARENT_SCOPE)
  set(names "${names}"
    PARENT_SCOPE)
  set(args "${args}"
    PARENT_SCOPE)
endfunction ()

#[==[.md INTERNAL
For handling global option settings, this function sets variables in the
calling scoped named `<PREFIX><OPTION>` to either `0` or `1` if the option is
present in the remaining argument list.

~~~
_vtk_test_set_options(<OPTIONS> <PREFIX> <ARG>...)
~~~

Additionally, a non-`0` default for a given option may be specified by a
variable with the same name as the option and specifying a prefix for the
output variables.
#]==]
function (_vtk_test_set_options options prefix)
  foreach (option IN LISTS options)
    set(default 0)
    if (prefix)
      set(default "${${option}}")
    endif ()
    set("${prefix}${option}" "${default}"
      PARENT_SCOPE)
  endforeach ()
  foreach (option IN LISTS ARGN)
    set("${prefix}${option}" 1
      PARENT_SCOPE)
  endforeach ()
endfunction ()

# If set, use the maximum number of processors for tests. Otherwise, just use 1
# processor by default.
set(VTK_MPI_NUMPROCS "2" CACHE STRING
  "Number of processors available to run parallel tests.")
# Hide the variable if we don't have `MPIEXEC_EXECUTABLE` anyways.
if (MPIEXEC_EXECUTABLE)
  set(_vtk_mpi_max_numprocs_type STRING)
else ()
  set(_vtk_mpi_max_numprocs_type INTERNAL)
endif ()
set_property(CACHE VTK_MPI_NUMPROCS
  PROPERTY
    TYPE "${_vtk_mpi_max_numprocs_type}")

#[==[.md
## C++ tests

This function declares C++ tests. Source files are required to use the `cxx`
extension.

~~~
vtk_add_test_cxx(<EXENAME> <VARNAME> <ARG>...)
~~~

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
`<VARNAME>` variable in the calling scope.

Options:

  - `NO_DATA`: The test does not need to know the test input data directory. If
    it does, it is passed on the command line via the `-D` flag.
  - `NO_VALID`: The test does not have a valid baseline image. If it does, the
    baseline is assumed to be in `../Data/Baseline/<NAME>.png` relative to the
    current source directory. If alternate baseline images are required,
    `<NAME>` may be suffixed by `_1`, `_2`, etc. The valid image is passed via
    the `-V` flag.
  - `NO_OUTPUT`: The test does not need to write out any data to the
    filesystem. If it does, a directory which may be written to is passed via
    the `-T` flag.

Additional flags may be passed to tests using the `${_vtk_build_test}_ARGS`
variable or the `<NAME>_ARGS` variable.
#]==]
function (vtk_add_test_cxx exename _tests)
  set(cxx_options
    NO_DATA
    NO_VALID
    NO_OUTPUT)
  _vtk_test_parse_args("${cxx_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${cxx_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${cxx_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name("${name}")

    set(_D "")
    if (NOT local_NO_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
    endif ()

    set(_T "")
    if (NOT local_NO_OUTPUT)
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
    endif ()

    set(_V "")
    if (NOT local_NO_VALID)
      set(_V -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${test_name}.png,:}")
    endif ()

    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME    "${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
      COMMAND "${exename}"
              "${test_file}"
              ${args}
              ${${_vtk_build_test}_ARGS}
              ${${name}_ARGS}
              ${_D} ${_T} ${_V})
    set_tests_properties("${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h
        SKIP_RETURN_CODE 125
      )

    list(APPEND ${_tests} "${test_file}")
  endforeach ()

  set("${_tests}" ${${_tests}} PARENT_SCOPE)
endfunction ()

#[==[.md
### MPI tests

This function declares C++ tests which should be run under an MPI environment.
Source files are required to use the `cxx` extension.

~~~
vtk_add_test_mpi(<EXENAME> <VARNAME> <ARG>...)
~~~

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
`<VARNAME>` variable in the calling scope.

Options:

  - `TESTING_DATA`
  - `NO_VALID`: The test does not have a valid baseline image. If it does, the
    baseline is assumed to be in `../Data/Baseline/<NAME>.png` relative to the
    current source directory. If alternate baseline images are required,
    `<NAME>` may be suffixed by `_1`, `_2`, etc. The valid image is passed via
    the `-V` flag.

Each test is run using the number of processors specified by the following
variables (using the first one which is set):

  - `<NAME>_NUMPROCS`
  - `<EXENAME>_NUMPROCS`
  - `VTK_MPI_NUMPROCS` (defaults to `2`)

Additional flags may be passed to tests using the `${_vtk_build_test}_ARGS`
variable or the `<NAME>_ARGS` variable.
#]==]
function (vtk_add_test_mpi exename _tests)
  set(mpi_options
    TESTING_DATA
    NO_VALID
    )
  _vtk_test_parse_args("${mpi_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${mpi_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  set(default_numprocs ${VTK_MPI_NUMPROCS})
  if (${exename}_NUMPROCS)
    set(default_numprocs ${${exename}_NUMPROCS})
  endif ()

  foreach (name IN LISTS names)
    _vtk_test_set_options("${mpi_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    set(_D "")
    set(_T "")
    set(_V "")
    if (local_TESTING_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
      set(_V "")
      if (NOT local_NO_VALID)
        set(_V -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${name}.png,:}")
      endif ()
    endif ()

    set(numprocs ${default_numprocs})
    if (${name}_NUMPROCS)
      set(numprocs "${${name}_NUMPROCS}")
    endif ()

    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME "${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}"
      COMMAND "${MPIEXEC_EXECUTABLE}"
              "${MPIEXEC_NUMPROC_FLAG}" "${numprocs}"
              ${MPIEXEC_PREFLAGS}
              "$<TARGET_FILE:${exename}>"
              "${test_file}"
              ${_D} ${_T} ${_V}
              ${args}
              ${${_vtk_build_test}_ARGS}
              ${${name}_ARGS}
              ${MPIEXEC_POSTFLAGS})
    set_tests_properties("${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        PROCESSORS "${numprocs}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h"
        SKIP_RETURN_CODE 125
      )
    set_property(TEST "${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}" APPEND
      PROPERTY
        REQUIRED_FILES "$<TARGET_FILE:${exename}>")
    list(APPEND ${_tests} "${test_file}")
  endforeach ()

  set(${_tests} ${${_tests}} PARENT_SCOPE)
endfunction ()

#[==[.md
### C++ test executable

~~~
vtk_test_cxx_executable(<EXENAME> <VARNAME> [RENDERING_FACTORY] [<SRC>...])
~~~

Creates an executable named `EXENAME` which contains the tests listed in the
variable named in the `VARNAME` argument. The `EXENAME` must match the
`EXENAME` passed to the test declarations when building the list of tests.

If `RENDERING_FACTORY` is provided, VTK's rendering factories are initialized
during the test.

Any additional arguments are added as additional sources for the executable.
#]==]
function (vtk_test_cxx_executable exename _tests)
  set(exe_options
    RENDERING_FACTORY)
  _vtk_test_parse_args("${exe_options}" "" ${ARGN})
  _vtk_test_set_options("${exe_options}" "" ${options})

  if (NOT ${_tests})
    # No tests -> no need for an executable.
    return()
  endif ()

  if (RENDERING_FACTORY)
    include("${_vtkModuleTesting_dir}/vtkTestingRenderingDriver.cmake")
    set(test_driver vtkTestingObjectFactory.h)
  else ()
    include("${_vtkModuleTesting_dir}/vtkTestingDriver.cmake")
    set(test_driver vtkTestDriver.h)
  endif ()

  set(extra_sources ${args})

  create_test_sourcelist(test_sources "${exename}.cxx" ${${_tests}}
    EXTRA_INCLUDE "${test_driver}")

  if (_vtk_build_test)
    vtk_module_test_executable("${exename}" ${test_sources} ${extra_sources})
  else ()
    message(FATAL_ERROR "_vtk_build_test is not set!")
  endif ()
endfunction ()

#[==[.md INTERNAL
MPI executables used to have their own test executable function. This is no
longer necessary and is deprecated. Instead, `vtk_test_cxx_executable` should
be used instead.
#]==]
function (vtk_test_mpi_executable exename _tests)
  message(DEPRECATION
    "The `vtk_test_mpi_executable` function is deprecated; use "
    "`vtk_test_cxx_executable` instead.")
  vtk_test_cxx_executable("${exename}" "${_tests}" ${ARGN})
endfunction ()

#[==[.md
## Python tests

This function declares Python tests. Test files are required to use the `py`
extension.

~~~
vtk_add_test_python(<EXENAME> <VARNAME> <ARG>...)
~~~
#]==]

#[==[.md INTERNAL
If the `_vtk_testing_python_exe` variable is not set, the `vtkpython` binary is
used by default. Additional arguments may be passed in this variable as well.
#]==]

#[==[.md
Options:

  - `NO_DATA`
  - `NO_VALID`
  - `NO_OUTPUT`
  - `NO_RT`
  - `JUST_VALID`

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
`<VARNAME>` variable in the calling scope.

Options:

  - `NO_DATA`: The test does not need to know the test input data directory. If
    it does, it is passed on the command line via the `-D` flag.
  - `NO_OUTPUT`: The test does not need to write out any data to the
    filesystem. If it does, a directory which may be written to is passed via
    the `-T` flag.
  - `NO_VALID`: The test does not have a valid baseline image. If it does, the
    baseline is assumed to be in `../Data/Baseline/<NAME>.png` relative to the
    current source directory. If alternate baseline images are required,
    `<NAME>` may be suffixed by `_1`, `_2`, etc. The valid image is passed via
    the `-V` flag.
  - `NO_RT`: If `NO_RT` is specified, `-B` is passed instead of `-V`, only
     providing a baseline dir, assuming `NO_VALID` is not specified.
  - `DIRECT_DATA` : If `DIRECT_DATA` is specified, the baseline path will be provided
     as is, without the use of ExternalData_add_test.
  - `JUST_VALID`: Only applies when both `NO_VALID` and `NO_RT` are not
    present. If it is not specified, `-A` is passed with path to the directory
    of the `vtkTclTest2Py` Python package and the test is run via the
    `rtImageTest.py` script. Note that this currently only works when building
    against a VTK build tree; the VTK install tree does not include this script
    or its associated Python package.

Additional flags may be passed to tests using the `${_vtk_build_test}_ARGS`
variable or the `<NAME>_ARGS` variable.

Note that the `vtkTclTest2Py` support will eventually be removed. It is a
legacy of the conversion of many tests from Tcl to Python.
#]==]
function (vtk_add_test_python)
  if (NOT _vtk_testing_python_exe)
    set(_vtk_testing_python_exe "$<TARGET_FILE:VTK::vtkpython>")
  endif ()
  set(python_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    NO_RT
    DIRECT_DATA
    JUST_VALID
    )
  _vtk_test_parse_args("${python_options}" "py" ${ARGN})
  _vtk_test_set_options("${python_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${python_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    set(_D "")
    if (NOT local_NO_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
    endif ()

    set(rtImageTest "")
    set(_B "")
    set(_V "")
    set(_A "")
    if (NOT local_NO_VALID)
      if (local_NO_RT)
        if (local_DIRECT_DATA)
          set(_B -B "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/")
        else ()
          set(_B -B "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/,REGEX:${test_name}(-.*)?(_[0-9]+)?.png}")
        endif()
      else ()
        if (local_DIRECT_DATA)
          set(_V -V "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/${test_name}.png")
        else ()
          set(_V -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${test_name}.png,:}")
        endif()
        if (NOT local_JUST_VALID)
          # TODO: This should be fixed to also work from an installed VTK.
          set(rtImageTest "${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py")
          set(_A -A "${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py")
        endif ()
      endif ()
    endif ()

    set(_T "")
    if (NOT local_NO_OUTPUT)
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
    endif ()

    if (NOT _vtk_build_TEST_FILE_DIRECTORY)
      set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    set(testArgs NAME "${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
                 COMMAND ${_vtk_test_python_pre_args}
                         "${_vtk_testing_python_exe}" ${_vtk_test_python_args} --enable-bt
                         ${rtImageTest}
                         "${_vtk_build_TEST_FILE_DIRECTORY}/${test_file}.py"
                         ${args}
                         ${${_vtk_build_test}_ARGS}
                         ${${name}_ARGS}
                         ${_D} ${_B} ${_T} ${_V} ${_A})

    if (local_DIRECT_DATA)
      add_test(${testArgs})
    else ()
      ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}" ${testArgs})
    endif()

    set_tests_properties("${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        # This must match the skip() function in vtk/test/Testing.py"
        SKIP_RETURN_CODE 125
      )
  endforeach ()
endfunction ()

#[==[.md
### MPI tests

A small wrapper around `vtk_add_test_python` which adds support for running
MPI-aware tests written in Python.

The `$<module library name>_NUMPROCS` variable may be used to use a non-default
number of processors for a test.

This forces running with the `pvtkpython` executable.
#]==]
function (vtk_add_test_python_mpi)
  set(_vtk_test_python_suffix "-MPI")

  set(numprocs "${VTK_MPI_NUMPROCS}")
  _vtk_module_get_module_property("${_vtk_build_test}"
    PROPERTY "library_name"
    VARIABLE _vtk_test_python_library_name)
  if (${_vtk_test_python_library_name}_NUMPROCS)
    set(numprocs "${${_vtk_test_python_library_name}_NUMPROCS}")
  endif ()

  set(_vtk_test_python_pre_args
    "${MPIEXEC_EXECUTABLE}"
    "${MPIEXEC_NUMPROC_FLAG}" "${numprocs}"
    ${MPIEXEC_PREFLAGS})

  if (NOT _vtk_testing_python_exe)
    set(_vtk_testing_python_exe "$<TARGET_FILE:VTK::pvtkpython>")
  endif ()
  vtk_add_test_python(${ARGN})
endfunction ()
