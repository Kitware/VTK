#[==[.rst:
vtkModuleTesting
----------------

VTK uses the ExternalData_ CMake module to handle the data management for
its test suite. Test data is only downloaded when a test which requires it is
enabled and it is cached so that every build does not need to redownload the
same data.

To facilitate this workflow, there are a number of CMake functions available in
order to indicate that test data is required.

.. _ExternalData:  https://cmake.org/cmake/help/latest/module/ExternalData.html
#]==]

include(ExternalData)
get_filename_component(_vtkModuleTesting_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

#[==[.rst:
Loading data
^^^^^^^^^^^^
.. cmake:command:: vtk_module_test_data

  Download test data. |module|

  Data may be downloaded manually using this function:

  .. code-block:: cmake

    vtk_module_test_data(<PATHSPEC>...)

  This will download data inside of the input data directory for the modules
  being built at that time (see the ``TEST_INPUT_DATA_DIRECTORY`` argument of
  ``vtk_module_build``).

  For supported `PATHSPEC` syntax, see the
  associated documentation in ref:`ExternalData`. These
  arguments are already wrapped in the ``DATA{}`` syntax and are assumed to be
  relative paths from the input data directory.

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

# Opt-in option from projects using VTK to activate SSIM baseline comparison
if (DEFINED DEFAULT_USE_SSIM_IMAGE_COMP AND DEFAULT_USE_SSIM_IMAGE_COMP)
  set(default_image_compare "VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
# We are compiling VTK standalone if we succeed the following condition
elseif (DEFINED VTK_VERSION)
  set(default_image_compare "VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
else()
  set(default_image_compare "VTK_TESTING_IMAGE_COMPARE_METHOD=LEGACY_VALID")
endif()

#[==[.rst:
Creating test executables
^^^^^^^^^^^^^^^^^^^^^^^^^

.. cmake:command:: vtk_module_test_executable

  This function creates an executable from the list of sources passed to it. It
  is automatically linked to the module the tests are intended for as well as any
  declared test dependencies of the module.

  .. code-block:: cmake

    vtk_module_test_executable(<NAME> <SOURCE>...)

  This function is not usually used directly, but instead through the other
  convenience functions.
#]==]
function (vtk_module_test_executable name)
  add_executable("${name}")
  target_sources("${name}"
    PRIVATE
      ${ARGN})
  get_property(test_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_test}_test_depends")
  get_property(test_optional_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_test}_test_optional_depends")
  set(optional_depends_flags)
  foreach (test_optional_depend IN LISTS test_optional_depends)
    _vtk_module_optional_dependency_exists("${test_optional_depend}"
      SATISFIED_VAR test_optional_depend_exists)
    if (test_optional_depend_exists)
      list(APPEND test_depends
        "${test_optional_depend}")
    endif ()
    string(REPLACE "::" "_" safe_test_optional_depend "${test_optional_depend}")
    list(APPEND optional_depends_flags
      "VTK_MODULE_ENABLE_${safe_test_optional_depend}=$<BOOL:${test_optional_depend_exists}>")
  endforeach ()

  if (_vtk_build_UTILITY_TARGET)
    target_link_libraries("${name}"
      PRIVATE
        "${_vtk_build_UTILITY_TARGET}")
  endif ()

  target_link_libraries("${name}"
    PRIVATE
      "${_vtk_build_test}"
      ${test_depends})
  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries("${name}"
      PRIVATE
        VTK::vtkWebAssemblyTestLinkOptions)
  endif ()
  target_compile_definitions("${name}"
    PRIVATE
      ${optional_depends_flags})
  vtk_module_autoinit(
    TARGETS "${name}"
    MODULES "${_vtk_build_test}"
            ${test_depends})
endfunction ()

#[==[.rst:
Test name parsing
^^^^^^^^^^^^^^^^^

Test names default to using the basename of the filename which contains the
test. Two tests may share the same file by prefixing with a custom name for the
test and a comma.

The two parsed syntaxes are:
- ``CustomTestName,TestFile``
- ``TestFile``

Note that ``TestFile`` should already have had its extension stripped (usually
done by ``_vtk_test_parse_args``).

In general, the name of a test will be ``<EXENAME>-<TESTNAME>``, however, by
setting ``vtk_test_prefix``, the test name will instead be
``<EXENAME>-<PREFIX><TESTNAME>``.
#]==]

#[==[.rst :
 .. cmake:command:: _vtk_test_parse_name


 This function parses the name from a testspec.|module-internal| The calling scope has
 `test_name`, `test_arg`, and `test_file` variables set in it.

 .. code-block:: cmake

   _vtk_test_parse_name(<TESTSPEC>)

#]==]
function (_vtk_test_parse_name name ext)
  if (name AND name MATCHES "^([^,]*),(.*)$")
    set(test_name "${CMAKE_MATCH_1}")
    set(test_file "${CMAKE_MATCH_2}")
  else ()
    # Strip the extension from the test name.
    string(REPLACE ".${ext}" "" test_name "${name}")
    set(test_name "${test_name}")
    set(test_file "${name}")
  endif ()

  string(REPLACE ".${ext}" "" test_arg "${test_file}")

  set(test_name "${test_name}" PARENT_SCOPE)
  set(test_file "${test_file}" PARENT_SCOPE)
  set(test_arg "${test_arg}" PARENT_SCOPE)
endfunction ()

#[==[.rst:
Test function arguments
^^^^^^^^^^^^^^^^^^^^^^^

Each test is specified  using one of the two following syntaxes

- ``<NAME>.<SOURCE_EXT>``
- ``<NAME>.<SOURCE_EXT>,<OPTIONS>``

Where ``NAME`` is a valid test name. If present, the specified ``OPTIONS`` are only
for the associated test. The expected extension is specified by the associated
test function.
#]==]

#[==[.rst:

.. cmake:command:: _vtk_test_parse_args

  |module-internal|
  Given a list of valid "options", this function will parse out a the following
  variables:

  - ``args``: Unrecognized arguments. These should be interpreted as arguments
    that should be passed on the command line to all tests in this parse group.
  - ``options``: Options specified globally (for all tests in this group).
  - ``names``: A list containing all named tests. These should be parsed by
    ``_vtk_test_parse_name``.
  - ``_<NAME>_options``: Options specific to a certain test.

  .. code-block:: cmake

    _vtk_test_parse_args(<OPTIONS> <SOURCE_EXT> <ARG>...)

  In order to be recognized as a source file, the ``SOURCE_EXT`` must be used.
  Without it, all non-option arguments are placed into ``args``. Each test is
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
    elseif (source_ext AND arg MATCHES "^([^.]*\\.${source_ext}),?(.*)$")
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

#[==[.rst:
.. cmake:command:: _vtk_test_set_options

  For handling global option settings |module-internal|, this function sets variables in the
  calling scoped named `<PREFIX><OPTION>` to either `0` or `1` if the option is
  present in the remaining argument list.

  .. code-block:: cmake

    _vtk_test_set_options(<OPTIONS> <PREFIX> <ARG>...)

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

#[==[.rst:
C++ tests
^^^^^^^^^

.. cmake:command:: vtk_add_test_cxx

  This function declares C++ tests |module|. Source files are required to use the `cxx`
  extension.

  .. code-block:: cmake

    vtk_add_test_cxx(<EXENAME> <VARNAME> <ARG>...)

  Each argument should be either an option, a test specification, or it is passed
  as flags to all tests declared in the group. The list of tests is set in the
  ``<VARNAME>`` variable in the calling scope.

  Options:

  - ``NO_DATA``: The test does not need to know the test input data directory. If
    it does, it is passed on the command line via the ``-D`` flag.
  - ``NO_VALID``: The test does not have a valid baseline image. If it does, the
    baseline is assumed to be in ``../Data/Baseline/<NAME>.png`` relative to the
    current source directory. If alternate baseline images are required,
    ``<NAME>`` may be suffixed by ``_1``, ``_2``, etc. The valid image is passed via
    the ``-V`` flag.
    - ``TIGHT_VALID``: Uses euclidean type metrics to compare baselines. Baseline
    comparison is sensitive to outliers in this setting.
    - ``LOOSE_VALID``: Uses L1 type metrics to compare baselines. Baseline comparison
    is somewhat more forgiving. Typical use cases involve rendering that is highly GPU
    dependent, and baselines with text.
    - ``LEGACY_VALID``: Uses legacy image compare. This metric generates a lot of
    false negatives. It is recommended not to use it.
  - ``NO_OUTPUT``: The test does not need to write out any data to the
    filesystem. If it does, a directory which may be written to is passed via
    the ``-T`` flag.

  Additional flags may be passed to tests using the ``${_vtk_build_test}_ARGS``
  variable or the ``<NAME>_ARGS`` variable.
#]==]
function (vtk_add_test_cxx exename _tests)
  set(cxx_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    TIGHT_VALID
    LOOSE_VALID
    LEGACY_VALID)
  _vtk_test_parse_args("${cxx_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${cxx_options}" "" ${options})

  set(_vtk_fail_regex
    # vtkLogger
    "(\n|^)ERROR: "
    "ERR\\|"
    # vtkDebugLeaks
    "instance(s)? still around"
    # vtkTesting
    "Failed Image Test"
    "DartMeasurement name=.ImageNotFound")

  set(_vtk_skip_regex
    # Insufficient graphics resources.
    "Attempt to use a texture buffer exceeding your hardware's limits"
    # Vulkan driver not setup correctly.
    "vulkan: No DRI3 support detected - required for presentation"
    # OpenGL driver cannot render wide lines.
    "a line width has been requested that is larger than your system supports")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${cxx_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name("${name}" "cxx")

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

    set(image_compare_method ${default_image_compare})
    if (local_LEGACY_VALID)
      set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LEGACY_VALID")
    elseif (local_LOOSE_VALID)
      set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LOOSE_VALID")
    elseif (local_TIGHT_VALID)
      set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
    endif ()

    set(vtk_testing "VTK_TESTING=1;${image_compare_method}")

    if (VTK_USE_MPI AND
        VTK_SERIAL_TESTS_USE_MPIEXEC)
      set(_vtk_test_cxx_pre_args
        "${MPIEXEC_EXECUTABLE}"
        "${MPIEXEC_NUMPROC_FLAG}" "1"
        ${MPIEXEC_PREFLAGS})
    endif()

    if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
      if (_vtk_test_cxx_wasm_enabled_in_browser)
        set(_vtk_test_cxx_pre_args
          "$<TARGET_FILE:Python3::Interpreter>"
          "${VTK_SOURCE_DIR}/Testing/WebAssembly/runner.py"
          "--engine=${VTK_TESTING_WASM_ENGINE}"
          "--exit")
      else ()
        ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
          NAME    "${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
          COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                  "--eval"
                  "process.exit(125);") # all tests are skipped.
        set_tests_properties("${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
          PROPERTIES
            LABELS "${_vtk_build_test_labels}"
            SKIP_RETURN_CODE 125 # This must match VTK_SKIP_RETURN_CODE in vtkTesting.h
          )
        continue ()
      endif ()
    endif ()
    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME    "${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
      COMMAND "${_vtk_test_cxx_pre_args}" "$<TARGET_FILE:${exename}>"
              "${test_arg}"
              "${args}"
              ${${_vtk_build_test}_ARGS}
              ${${test_name}_ARGS}
              ${_D} ${_T} ${_V})
    set_tests_properties("${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        SKIP_REGULAR_EXPRESSION "${_vtk_skip_regex}"
        # Disables anti-aliasing when rendering
        ENVIRONMENT "${vtk_testing}"
        SKIP_RETURN_CODE 125 # This must match VTK_SKIP_RETURN_CODE in vtkTesting.h
      )

    if (_vtk_testing_ld_preload)
      set_property(TEST "${_vtk_build_test}Cxx-${vtk_test_prefix}${test_name}" APPEND
        PROPERTY
          ENVIRONMENT "LD_PRELOAD=${_vtk_testing_ld_preload}")
    endif ()
    list(APPEND ${_tests} "${test_file}")
  endforeach ()

  set("${_tests}" ${${_tests}} PARENT_SCOPE)
endfunction ()

#[==[.rst:
MPI tests
"""""""""

.. cmake:command:: vtk_add_test_mpi

  This function declares C++ tests which should be run under an MPI environment. |module|
  Source files are required to use the `cxx` extension.

  .. code-block:: cmake

    vtk_add_test_mpi(<EXENAME> <VARNAME> <ARG>...)

  Each argument should be either an option, a test specification, or it is passed
  as flags to all tests declared in the group. The list of tests is set in the
  ``<VARNAME>`` variable in the calling scope.

  Options:

  - ``TESTING_DATA``
  - ``NO_VALID``: The test does not have a valid baseline image. If it does, the
    baseline is assumed to be in ``../Data/Baseline/<NAME>.png`` relative to the
    current source directory. If alternate baseline images are required,
    ``<NAME>`` may be suffixed by ``_1``, ``_2``, etc. The valid image is passed via
    the ``-V`` flag.

  Each test is run using the number of processors specified by the following
  variables (using the first one which is set):

  - ``<NAME>_NUMPROCS``
  - ``<EXENAME>_NUMPROCS``
  - ``VTK_MPI_NUMPROCS`` (defaults to ``2``)

  Additional flags may be passed to tests using the ``${_vtk_build_test}_ARGS``
  variable or the `<NAME>_ARGS` variable.
#]==]
function (vtk_add_test_mpi exename _tests)
  set(mpi_options
    TESTING_DATA
    NO_VALID
    LOOSE_VALID
    TIGHT_VALID
    LEGACY_VALID
    )
  _vtk_test_parse_args("${mpi_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${mpi_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "ERR\\|" "instance(s)? still around")

  set(_vtk_skip_regex
    # Insufficient graphics resources.
    "Attempt to use a texture buffer exceeding your hardware's limits"
    # Vulkan driver not setup correctly.
    "vulkan: No DRI3 support detected - required for presentation")

  set(default_numprocs ${VTK_MPI_NUMPROCS})
  if (${exename}_NUMPROCS)
    set(default_numprocs ${${exename}_NUMPROCS})
  endif ()

  foreach (name IN LISTS names)
    _vtk_test_set_options("${mpi_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name} "cxx")

    set(_D "")
    set(_T "")
    set(_V "")
    set(image_compare_method ${default_image_compare})
    if (local_TESTING_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
      set(_V "")
      if (NOT local_NO_VALID)
        set(_V -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${test_name}.png,:}")
      endif ()
      if (local_LEGACY_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LEGACY_VALID")
      elseif (local_LOOSE_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LOOSE_VALID")
      elseif (local_TIGHT_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
      endif ()
    endif ()

    set(vtk_testing "VTK_TESTING=1;${image_compare_method}")

    set(numprocs ${default_numprocs})
    if (${test_name}_NUMPROCS)
      set(numprocs "${${test_name}_NUMPROCS}")
    endif ()

    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME "${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}"
      COMMAND "${MPIEXEC_EXECUTABLE}"
              "${MPIEXEC_NUMPROC_FLAG}" "${numprocs}"
              ${MPIEXEC_PREFLAGS}
              "$<TARGET_FILE:${exename}>"
              "${test_arg}"
              ${_D} ${_T} ${_V}
              ${args}
              ${${_vtk_build_test}_ARGS}
              ${${test_name}_ARGS})
    set_tests_properties("${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        PROCESSORS "${numprocs}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        SKIP_REGULAR_EXPRESSION "${_vtk_skip_regex}"
        ENVIRONMENT "${vtk_testing}"
        SKIP_RETURN_CODE 125
      )

    if (_vtk_testing_ld_preload)
      set_property(TEST "${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}" APPEND
        PROPERTY
          ENVIRONMENT "LD_PRELOAD=${_vtk_testing_ld_preload}")
    endif ()

    set_property(TEST "${_vtk_build_test}Cxx-MPI-${vtk_test_prefix}${test_name}" APPEND
      PROPERTY
        REQUIRED_FILES "$<TARGET_FILE:${exename}>")
    list(APPEND ${_tests} "${test_file}")
  endforeach ()

  set(${_tests} ${${_tests}} PARENT_SCOPE)
endfunction ()

#[==[.rst:

C++ test executable
^^^^^^^^^^^^^^^^^^^

.. cmake:command:: vtk_test_cxx_executable

  .. code-block:: cmake

    vtk_test_cxx_executable(<EXENAME> <VARNAME> [RENDERING_FACTORY] [<SRC>...])

  Creates an executable named ``EXENAME`` which contains the tests listed in the
  variable named in the ``VARNAME`` argument. The ``EXENAME`` must match the
  ``EXENAME`` passed to the test declarations when building the list of tests.

  If ``RENDERING_FACTORY`` is provided, VTK's rendering factories are initialized
  during the test.

  By default, VTK's rendering tests enable FP exceptions to find floating point
  errors in debug builds. If ``DISABLE_FLOATING_POINT_EXCEPTIONS`` is provided,
  FP exceptions are not enabled for the test. This is useful when testing against
  external libraries to ignore exceptions in third-party code.

  Any additional arguments are added as additional sources for the executable.
#]==]
function (vtk_test_cxx_executable exename _tests)
  set(exe_options
    RENDERING_FACTORY
    DISABLE_FLOATING_POINT_EXCEPTIONS
    )
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

#[==[.rst:

.. cmake:command:: vtk_test_mpi_executable

MPI executables used to have their own test executable function.|module-internal| This is no
longer necessary and is deprecated. Instead, `vtk_test_cxx_executable` should
be used instead.
#]==]
function (vtk_test_mpi_executable exename _tests)
  message(DEPRECATION
    "The `vtk_test_mpi_executable` function is deprecated; use "
    "`vtk_test_cxx_executable` instead.")
  vtk_test_cxx_executable("${exename}" "${_tests}" ${ARGN})
endfunction ()

#[==[.rst:

Python tests
^^^^^^^^^^^^

.. cmake:command:: vtk_add_test_python

  This function declares Python tests.|module| Test files are required to use the `py`
  extension.

  .. code-block:: cmake

    vtk_add_test_python(<EXENAME> <VARNAME> <ARG>...)
#]==]

#[==[.rst:
If the ``_vtk_testing_python_exe`` variable is not set, the ``vtkpython`` binary is
used by default. Additional arguments may be passed in this variable as well.

If the given Python executable supports VTK's ``--enable-bt`` flag, setting
``_vtk_testing_python_exe_supports_bt`` to ``1`` is required to enable it.
#]==]

#[==[.rst
Options:

- ``NO_DATA``
- ``NO_VALID``
- ``NO_OUTPUT``
- ``NO_RT``
- ``JUST_VALID``
- ``LEGACY_VALID``
- ``TIGHT_VALID``
- ``LOOSE_VALID``

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
``<VARNAME>`` variable in the calling scope.

Options:

- ``NO_DATA``: The test does not need to know the test input data directory. If
  it does, it is passed on the command line via the ``-D`` flag.
- ``NO_OUTPUT``: The test does not need to write out any data to the
  filesystem. If it does, a directory which may be written to is passed via
  the ``-T`` flag.
- ``NO_VALID``: The test does not have a valid baseline image. If it does, the
  baseline is assumed to be in ``../Data/Baseline/<NAME>.png`` relative to the
  current source directory. If alternate baseline images are required,
  ``<NAME>`` may be suffixed by ``_1``, ``_2``, etc. The valid image is passed via
  the ``-V`` flag.
- ``NO_RT``: If ``NO_RT`` is specified, ``-B`` is passed instead of ``-V``, only
   providing a baseline dir, assuming ``NO_VALID`` is not specified.
- ``DIRECT_DATA`` : If ``DIRECT_DATA`` is specified, the baseline path will be provided
   as is, without the use of ExternalData_add_test.
- ``JUST_VALID``: Only applies when neither ``NO_VALID`` or ``NO_RT`` are present.
  If it is not specified, the test is run via ``vtkmodules.test.rtImageTest``.
- ``TIGHT_VALID``: Default behavior if legacy image comparison method is turned off by default.
  The baseline is tested using an euclidean metric, which is sensitive to outliers.
- ``LOOSE_VALID``: The baseline is tested using an norm-1 metric, which is less sensitive to
  outliers. It should typically be used when comparing text or when testing rendering that
  varies a lot depending on the GPU drivers.
- ``LEGACY_VALID``: Uses legacy image compare metric, which is more forgiving than the new one.

Additional flags may be passed to tests using the ``${_vtk_build_test}_ARGS``
variable or the ``<NAME>_ARGS`` variable.
To use a different baseline name than ``<NAME>`` you can set
``<NAME>_BASELINE_NAME`` variable. This is the name of the image file without extension.
#]==]
function (vtk_add_test_python)
  if (NOT _vtk_testing_python_exe)
    set(_vtk_testing_python_exe "$<TARGET_FILE:VTK::vtkpython>")
    set(_vtk_testing_python_exe_supports_bt 1)
  endif ()
  set(python_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    NO_RT
    DIRECT_DATA
    JUST_VALID
    LEGACY_VALID
    TIGHT_VALID
    LOOSE_VALID
    )
  _vtk_test_parse_args("${python_options}" "py" ${ARGN})
  _vtk_test_set_options("${python_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "ERR\\|" "instance(s)? still around|DeprecationWarning")

  set(_vtk_skip_regex
    # Insufficient graphics resources.
    "Attempt to use a texture buffer exceeding your hardware's limits"
    # Vulkan driver not setup correctly.
    "vulkan: No DRI3 support detected - required for presentation")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${python_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name} "py")

    set(_D "")
    if (NOT local_NO_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
    endif ()

    set(rtImageTest "")
    set(_B "")
    set(_V "")
    set(image_compare_method ${default_image_compare})
    if (NOT local_NO_VALID)
      if (DEFINED "${test_name}_BASELINE_NAME")
        set(baseline_name "${${test_name}_BASELINE_NAME}")
      else()
        set(baseline_name "${test_name}")
      endif()
      if (local_NO_RT)
        if (local_DIRECT_DATA)
          set(_B -B "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/")
        else ()
          set(_B -B "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/,REGEX:${baseline_name}(-.*)?(_[0-9]+)?.png}")
        endif()
      else ()
        if (local_DIRECT_DATA)
          set(_V -V "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/${baseline_name}.png")
        else ()
          set(_V -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${baseline_name}.png,:}")
        endif()
        if (NOT local_JUST_VALID)
          set(rtImageTest -m "vtkmodules.test.rtImageTest")
        endif ()
      endif ()
      if (local_LEGACY_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LEGACY_VALID")
      elseif (local_TIGHT_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
      elseif (local_LOOSE_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LOOSE_VALID")
      endif ()
    endif ()

    set(_T "")
    if (NOT local_NO_OUTPUT)
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
    endif ()

    if (NOT _vtk_build_TEST_FILE_DIRECTORY)
      set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if (VTK_USE_MPI AND
        VTK_SERIAL_TESTS_USE_MPIEXEC AND
        NOT DEFINED _vtk_test_python_pre_args)
      set(_vtk_test_python_pre_args
        "${MPIEXEC_EXECUTABLE}"
        "${MPIEXEC_NUMPROC_FLAG}" "1"
        ${MPIEXEC_PREFLAGS})
    endif()
    set(_vtk_test_python_bt_args)
    if (_vtk_testing_python_exe_supports_bt)
      list(APPEND _vtk_test_python_bt_args --enable-bt)
    endif ()
    set(testArgs NAME "${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
                 COMMAND ${_vtk_test_python_pre_args}
                         "${_vtk_testing_python_exe}" ${_vtk_test_python_args}
                         ${_vtk_test_python_bt_args}
                         ${rtImageTest}
                         "${_vtk_build_TEST_FILE_DIRECTORY}/${test_file}"
                         ${args}
                         ${${_vtk_build_test}_ARGS}
                         ${${test_name}_ARGS}
                         ${_D} ${_B} ${_T} ${_V})

    if (local_DIRECT_DATA)
      add_test(${testArgs})
    else ()
      ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}" ${testArgs})
    endif()

    if (_vtk_testing_ld_preload)
      set_property(TEST "${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
        APPEND
        PROPERTY
          ENVIRONMENT "LD_PRELOAD=${_vtk_testing_ld_preload}")
    endif ()

    set_tests_properties("${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        SKIP_REGULAR_EXPRESSION "${_vtk_skip_regex}"
        ENVIRONMENT "VTK_TESTING=1;${image_compare_method}"
        # This must match the skip() function in vtk/test/Testing.py"
        SKIP_RETURN_CODE 125
      )

    if (numprocs)
      set_tests_properties("${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
        PROPERTIES
          PROCESSORS "${numprocs}")
    endif ()
  endforeach ()
endfunction ()

#[==[.rst:
JavaScript tests
^^^^^^^^^^^^^^^^^^^^^^^^^

.. cmake:command:: vtk_add_test_module_javascript_node

  This function declares JavaScript tests run with NodeJS.
  Test files are required to use the `mjs` extension.
  Additional arguments to `node` can be passed via `_vtk_node_args` variable.

  .. code-block:: cmake

    vtk_add_test_module_javascript_node(<VARNAME> <ARG>...)
#]==]

#[==[.rst:
The ``_vtk_testing_nodejs_exe`` variable must point to the path of a `node` interpreter.
#]==]

#[==[.rst
Options:

- ``NO_DATA``
- ``NO_OUTPUT``

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
``<VARNAME>`` variable in the calling scope.

Options:

- ``NO_DATA``: The test does not need to know the test input data directory. If
  it does, it is passed on the command line via the ``-D`` flag.
- ``NO_OUTPUT``: The test does not need to write out any data to the
  filesystem. If it does, a directory which may be written to is passed via
  the ``-T`` flag.

Additional flags may be passed to tests using the ``${_vtk_build_test}_ARGS``
variable or the ``<NAME>_ARGS`` variable.
#]==]
function (vtk_add_test_module_javascript_node)
  if (NOT _vtk_testing_nodejs_exe)
    message(FATAL_ERROR "The \"_vtk_testing_nodejs_exe\" variable must point to a nodejs executable!")
  endif ()
  set(mjs_options
    NO_DATA
    NO_OUTPUT)
  _vtk_test_parse_args("${mjs_options}" "mjs" ${ARGN})
  _vtk_test_set_options("${mjs_options}" "" ${options})

  set(_vtk_fail_regex
    # vtkLogger
    "(\n|^)ERROR: "
    "ERR\\|"
    # vtkDebugLeaks
    "instance(s)? still around")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${mjs_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name("${name}" "mjs")
    set(_D "")
    if (NOT local_NO_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
    endif ()

    set(_T "")
    if (NOT local_NO_OUTPUT)
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
    endif ()
    ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
      NAME    "${_vtk_build_test}JavaScript-${test_name}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND ${_vtk_testing_nodejs_exe}
              "${_vtk_node_args}"
              "${test_file}"
              ${${_vtk_build_test}_ARGS}
              ${${test_name}_ARGS}
              ${_D} ${_T})
    set_tests_properties("${_vtk_build_test}JavaScript-${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        SKIP_RETURN_CODE 125)
  endforeach ()
endfunction ()

#[==[.rst:
MPI tests
"""""""""

.. cmake:command:: vtk_add_test_python_mpi

  A small wrapper around ``vtk_add_test_python`` which adds support for running
  MPI-aware tests written in Python.

  The ``$<module library name>_NUMPROCS`` variable may be used to use a non-default
  number of processors for a test.

  This forces running with the ``pvtkpython`` executable.
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
    set(_vtk_testing_python_exe_supports_bt 1)
  endif ()
  vtk_add_test_python(${ARGN})
endfunction ()

#[==[.rst:
ABI Mangling tests
""""""""""""""""""

.. cmake:command:: vtk_add_test_mangling

This function declares a test to verify that all of the exported symbols in the
VTK module library contain the correct ABI mangling prefix. This test requires
setting the option VTK_ABI_NAMESPACE_NAME to a value that is not "<DEFAULT>".

Current limitations of this test are:
- Does not run on non-UNIX platforms
- Is not compatible with the option "VTK_ENABLE_KITS"
- May not work outside of VTK itself

  .. code-block:: cmake

    vtk_add_test_mangling(module_name [EXEMPTIONS ...])

Options:
- ``EXEMPTIONS``: List of symbol patterns to excluded from the ABI mangling test
  where it is known that the symbols do not support the ABI mangling but are still
  exported. This option should be extremely rare to use, see the documentation on ABI
  mangling for how the handle C and C++ symbols before adding an EXEMPTION.
#]==]
function (vtk_add_test_mangling module)
  get_property(vtk_abi_namespace_name GLOBAL PROPERTY _vtk_abi_namespace_name)
  if (vtk_abi_namespace_name STREQUAL "")
    return()
  endif ()

  get_property(_vtkmoduletesting_nomangle_warnings_isset GLOBAL PROPERTY vtkmoduletesting_nomangle_warnings SET)
  if (NOT _vtkmoduletesting_nomangle_warnings_isset)
    set_property(GLOBAL PROPERTY vtkmoduletesting_nomangle_warnings FALSE)
  endif ()
  get_property(_vtkmoduletesting_nomangle_warnings GLOBAL PROPERTY vtkmoduletesting_nomangle_warnings)

  if (_vtkmoduletesting_nomangle_warnings)
    # Only warn on these issues once
    return()
  endif ()

  if (VTK_ENABLE_KITS)
    set(_vtkmoduletesting_nomangle_warnings TRUE)
    message(WARNING "Mangling tests are not supported with VTK_ENABLE_KITS (https://gitlab.kitware.com/vtk/vtk/-/issues/19207)")
  endif ()

  if (NOT UNIX)
    set(_vtkmoduletesting_nomangle_warnings TRUE)
    message(WARNING "Mangling tests are not supported on non-UNIX platforms")
  endif ()

  if (_vtkmoduletesting_nomangle_warnings)
    set_property(GLOBAL PROPERTY vtkmoduletesting_nomangle_warnings "${_vtkmoduletesting_nomangle_warnings}")
    return()
  endif ()

  cmake_parse_arguments(_vtk_mangling_test "" "" "EXEMPTIONS" ${ARGN})

  _vtk_module_real_target(_vtk_test_target "${module}")
  if (CMAKE_VERSION VERSION_LESS "3.19")
    get_property(target_type TARGET ${_vtk_test_target} PROPERTY TYPE)
    # CMake 3.19 introduced support for regular properties on `INTERFACE`
    # libraries. Before that, it was an error to ask for properties like
    # `SOURCES`. Avoid the error in this case (there aren't any objects
    # anyways, so no need to make any noise).
    if (target_type STREQUAL "INTERFACE_LIBRARY")
      return()
    endif ()
  endif ()
  get_property(has_sources TARGET ${_vtk_test_target} PROPERTY SOURCES)
  get_property(has_test GLOBAL PROPERTY "${module}_HAS_MANGLING_TEST" SET)

  if (NOT has_test AND has_sources)
    set_property(GLOBAL PROPERTY "${module}_HAS_MANGLING_TEST" 1)
    add_test(
      NAME    "${module}-ManglingTest"
      COMMAND "${Python3_EXECUTABLE}"
              # TODO: What to do when using this from a VTK install?
              "${VTK_SOURCE_DIR}/Testing/Core/CheckSymbolMangling.py"
              "--files"
              "$<TARGET_FILE:${_vtk_test_target}>"
              "--prefix"
              # TODO: This is not included in vtk-config.
              "${vtk_abi_namespace_name}"
              "--exemptions"
              "${_vtk_mangling_test_EXEMPTIONS}")
    set_property(TEST "${module}-ManglingTest" APPEND PROPERTY LABELS MANGLING)
  endif ()
endfunction ()
