# -----------------------------------------------------------------------------
# _vtk_test_parse_name(name)
#   INTERNAL: Parse the name of the test and the test file.
#
#   The 'name' argument must be of the form
#
#   [CustomTestName,]Test
#
#   where the CustomTestName followed by comma is optional. For example,
#   if 'name' has the value
#
#   Test1,Test
#
#   this function sets the variable 'test_name' to 'Test1' and
#   'test_file' to 'Test' in the parent scope. Note that the test file
#   does not include the file extension. For tests specified without a
#   custom name, .e.g.,
#
#   Test
#
#   both variables 'test_name' and 'test_file' will be set to the variable
#   'name' defined in the caller.
function(_vtk_test_parse_name name)
  set(test_name ${name} PARENT_SCOPE)
  set(test_file ${name} PARENT_SCOPE)

  if(name AND "x${name}" MATCHES "^x([^,]*),(.*)$")
    set(test_name "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(test_file "${CMAKE_MATCH_2}" PARENT_SCOPE)
  endif()
endfunction()

# -----------------------------------------------------------------------------
# _vtk_test_parse_args(options source_ext args...)
#   INTERNAL: Parse arguments for testing functions.
#
#   Parses 'options' from the argument list into the 'options' variable in the
#   parent, Test instances found with the extension 'source_ext' are parsed
#   into the 'names' variable in the parent. Any comma-separated options after
#   the test instance is put into a '_${name}_options' variable for the test.
#   Any unrecognized arguments are put into the 'args' variable in the parent.
function(_vtk_test_parse_args options source_ext)
  set(global_options)
  set(names)
  set(args)

  foreach(arg IN LISTS ARGN)
    set(handled 0)
    foreach(option IN LISTS options)
      if(arg STREQUAL option)
        list(APPEND global_options ${option})
        set(handled 1)
        break()
      endif()
    endforeach()
    if(handled)
      # Do nothing.
    elseif(source_ext AND "x${arg}" MATCHES "^x([^.]*)\\.${source_ext},?(.*)$")
      set(name "${CMAKE_MATCH_1}")
      string(REPLACE "," ";" _${name}_options "${CMAKE_MATCH_2}")
      list(APPEND names ${name})
    else()
      list(APPEND args ${arg})
    endif()
  endforeach()

  foreach(name IN LISTS names)
    set(_${name}_options "${_${name}_options}"
      PARENT_SCOPE)
  endforeach()
  set(options "${global_options}"
    PARENT_SCOPE)
  set(names "${names}"
    PARENT_SCOPE)
  set(args "${args}"
    PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# _vtk_test_set_options(options prefix args...)
#   INTERNAL: Set variables related to options.
#
#   Looks in the arguments for options to set. Valid options are listed in the
#   'options' input list and the variables of the same name are set in the
#   parent scope to '1' if set, and '0' if they are not found. If 'prefix' is
#   non-empty, it is used as a prefix for the variable names to set and the
#   no-prefix variable is used as the unset value (rather than '0').
function(_vtk_test_set_options options prefix)
  foreach(option IN LISTS options)
    set(default 0)
    if(prefix)
      set(default ${${option}})
    endif()
    set(${prefix}${option} ${default}
      PARENT_SCOPE)
  endforeach()
  foreach(option IN LISTS ARGN)
    set(${prefix}${option} 1
      PARENT_SCOPE)
  endforeach()
endfunction()

# -----------------------------------------------------------------------------
# vtk_add_test_mpi(exename tests [TESTING_DATA] [test1.cxx...] [args...])
#   Adds (C++) tests which require MPI.
#
#   Adds tests using the 'exename' (which must be a CMake target) and the name
#   of the tests into the variable named by 'tests' in the parent scope. If the
#   TESTING_DATA option is specified, -D, -T, and -V flags are passed to the
#   test. The number of processes to be used may be set with
#   ${exename}_NUMPROCS or ${test}_NUMPROCS to override the default
#   ${VTK_MPI_MAX_NUMPROCS} if necessary. Any unrecognized arguments are passed
#   to the test as well as the value of '${name}_ARGS. By default, the test
#   name will be the part of the source file before the '.cxx'. A custom test name
#   can be specified by giving a name followed by a comma before the test file
#   name, .e.g.,
#
#   CustomTestName,TestSource.cxx
#
#   The 'vtk_test_prefix' variable may also be set to create separate tests from a
#   single test name (e.g., running with different arguments), but should be
#   used only when required.
function(vtk_add_test_mpi exename _tests)
  set(mpi_options
    TESTING_DATA
    CUSTOM_BASELINES
    NO_VALID
    )
  _vtk_test_parse_args("${mpi_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${mpi_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  set(default_numprocs ${VTK_MPI_MAX_NUMPROCS})
  if(${exename}_NUMPROCS)
    set(default_numprocs ${${exename}_NUMPROCS})
  endif()

  set(data_dir "${VTK_TEST_DATA_DIR}")
  if(${vtk-module}_DATA_DIR)
    set(data_dir "${${vtk-module}_DATA_DIR}")
  endif()

  set(baseline_dir ${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline)
  if(VTK_BASELINE_DIR)
    set(baseline_dir ${VTK_BASELINE_DIR})
  endif()

  set(externaldata_target VTKData)
  if(VTK_TEST_DATA_TARGET)
    set(externaldata_target ${VTK_TEST_DATA_TARGET})
  endif()

  foreach(name IN LISTS names)
    _vtk_test_set_options("${mpi_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    set(_D "")
    set(_T "")
    set(_V "")
    if(local_TESTING_DATA)
      set(_D -D ${data_dir})
      set(_T -T ${VTK_TEST_OUTPUT_DIR})
      set(_V "")
      if(NOT local_NO_VALID)
        if(local_CUSTOM_BASELINES)
          set(_V -V "${data_dir}/Baseline")
        else()
          set(_V -V "DATA{${baseline_dir}/${test_name}.png,:}")
        endif()
      endif()
    endif()

    set(numprocs ${default_numprocs})
    if(${name}_NUMPROCS)
      set(numprocs ${${name}_NUMPROCS})
    endif()

    ExternalData_add_test(${externaldata_target}
      NAME ${vtk-module}Cxx-MPI-${vtk_test_prefix}${test_name}
      COMMAND ${VTK_MPIRUN_EXE}
              ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${numprocs}
              ${VTK_MPI_PREFLAGS}
              $<TARGET_FILE:${exename}>
              ${name}
              ${_D} ${_T} ${_V}
              ${args}
              ${${vtk-module}_ARGS}
              ${${name}_ARGS}
              ${VTK_MPI_POSTFLAGS})
    set_tests_properties(${vtk-module}Cxx-MPI-${vtk_test_prefix}${test_name}
      PROPERTIES
        LABELS "${${vtk-module}_TEST_LABELS}"
        PROCESSORS ${numprocs}
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
      )
    list(APPEND ${_tests} "${test_file}")

  endforeach()

  set(${_tests} ${${_tests}} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# vtk_test_mpi_executable(exename tests [RENDERING_FACTORY] [extra.cxx...])
#   Creates an MPI-aware C++ test executable.
#
#   Creates a test executable which uses MPI and contains the tests listed in
#   the variable 'tests'. See also 'vtk_test_cxx_executable'.
function(vtk_test_mpi_executable exename _tests)
  vtk_test_cxx_executable("${exename}" "${_tests}" ${ARGN})
  if(TARGET "${exename}")
    vtk_mpi_link("${exename}")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# vtk_add_test_cxx(exename tests [NO_DATA] [NO_VALID] [NO_OUTPUT]
#                  [test1.cxx...] [args...])
#   Adds C++ tests.
#
#   Adds tests using the 'exename' (which must be a CMake target) and the name
#   of the tests into the variable named by 'tests' in the parent scope. If the
#   NO_DATA option is specified, the test will not receive a -D argument (input file),
#   NO_VALID will suppress the -V argument (path to a baseline image), and
#   NO_OUTPUT will suppress the -T argument (output directory). Test-specific
#   arguments may be set to _${name}_ARGS. By default, the test name will be the part
#   of the source file before the '.cxx'. A custom test name can be specified by
#   giving a name followed by a comma before the test file name, .e.g.,
#
#   CustomTestName,TestSource.cxx
#
#   The 'vtk_test_prefix' variable may be set to create separate tests from a
#   single test name (e.g., running with different arguments), but should be
#   used only when required.
function(vtk_add_test_cxx exename _tests)
  set(cxx_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    CUSTOM_BASELINES
    )
  _vtk_test_parse_args("${cxx_options}" "cxx" ${ARGN})
  _vtk_test_set_options("${cxx_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  if(VTK_BASELINE_DIR)
    if(vtk-module)
      set(prefix ${vtk-module})
    elseif(vtk-example)
      set(prefix ${vtk-example})
    endif()
    set(baseline_dir ${VTK_BASELINE_DIR})
  elseif(vtk-module)
    set(prefix ${vtk-module})
    set(baseline_dir ${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline)
  elseif(vtk-example)
    set(prefix ${vtk-example})
    set(baseline_dir ${CMAKE_CURRENT_SOURCE_DIR}/Baseline)
  else()
    message(FATAL_ERROR "Neither vtk-module nor vtk-example is set!")
  endif()

  set(data_dir "${VTK_TEST_DATA_DIR}")
  if(${vtk-module}_DATA_DIR)
    set(data_dir "${${vtk-module}_DATA_DIR}")
  endif()

  set(externaldata_target VTKData)
  if(VTK_TEST_DATA_TARGET)
    set(externaldata_target ${VTK_TEST_DATA_TARGET})
  endif()

  foreach(name IN LISTS names)
    _vtk_test_set_options("${cxx_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    set(_D "")
    if(NOT local_NO_DATA)
      set(_D -D ${data_dir})
    endif()

    set(_T "")
    if(NOT local_NO_OUTPUT)
      set(_T -T ${VTK_TEST_OUTPUT_DIR})
    endif()

    set(_V "")
    if(NOT local_NO_VALID)
      if(local_CUSTOM_BASELINES)
        set(_V -V "${data_dir}/Baseline")
      else()
        set(_V -V "DATA{${baseline_dir}/${test_name}.png,:}")
      endif()
    endif()

    ExternalData_add_test(${externaldata_target}
      NAME    ${prefix}Cxx-${vtk_test_prefix}${test_name}
      COMMAND $<TARGET_FILE:${exename}>
              ${test_file}
              ${args}
              ${${prefix}_ARGS}
              ${${name}_ARGS}
              ${_D} ${_T} ${_V})
    set_tests_properties(${prefix}Cxx-${vtk_test_prefix}${test_name}
      PROPERTIES
        LABELS "${${prefix}_TEST_LABELS}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        # This must match VTK_SKIP_RETURN_CODE in vtkTestingObjectFactory.h"
        SKIP_RETURN_CODE 125
      )

    list(APPEND ${_tests} "${test_file}")
  endforeach()

  set(${_tests} ${${_tests}} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# vtk_test_cxx_executable(exename, tests [RENDERING_FACTORY] [extra.cxx...])
#   Build a C++ test executable.
#
#   Creates a test executable for running the tests listed in the 'tests'
#   variable. If RENDERING_FACTORY is set, the rendering test driver will be
#   used instead. Any other sources found will be built into the executable as
#   well. Unrecognized arguments are ignored.
function(vtk_test_cxx_executable exename _tests)
  set(exe_options
    RENDERING_FACTORY
    )
  _vtk_test_parse_args("${exe_options}" "" ${ARGN})
  _vtk_test_set_options("${exe_options}" "" ${options})

  if(NOT ${_tests})
    # No tests -> no need for an executable.
    return()
  endif()

  set(test_driver vtkTestDriver.h)
  if(RENDERING_FACTORY)
    include(vtkTestingRenderingDriver)
    set(test_driver ${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.h)
  endif()

  set(extra_sources ${args})

  if(vtk-module)
    set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
      "    vtksys::SystemInformation::SetStackTraceOnError(1);\n ${CMAKE_TESTDRIVER_BEFORE_TESTMAIN}")
  endif()

  create_test_sourcelist(test_sources ${exename}.cxx ${${_tests}}
    EXTRA_INCLUDE ${test_driver})

  if(vtk-module)
    vtk_module_test_executable(${exename} ${test_sources} ${extra_sources})
  elseif(vtk-example)
    add_executable(${exename} ${test_sources} ${extra_sources})
    target_link_libraries(${exename} ${VTK_LIBRARIES})
  else()
    message(FATAL_ERROR "Neither vtk-module nor vtk-example is set!")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# vtk_add_test_python([NO_RT] [NO_DATA] [NO_VALID] [NO_OUTPUT]
#                     [test1.py...] [args...])
#   Adds Python tests.
#
#   Adds Python tests to run. If NO_DATA is set, the -D argument to the test
#   (input data) will not be passed, NO_OUTPUT suppresses the -T argument
#   (output directory), NO_VALID will suppress the -B argument (baseline for
#   normal image comparisons (NO_RT)) and the -V and -A arguments (for RT-based
#   image comparisons). Test-specific arguments may be set to _${name}_ARGS. By
#   default, the test name will be the part of the source file before the '.py'.
#   A custom test name can be specified by giving a name followed by a comma
#   before the test file name, .e.g.,
#
#   CustomTestName,TestSource.py
#
#   The 'vtk_test_prefix' variable may be set to create separate tests from a
#   single test name (e.g., running with different arguments), but should be
#   used only when required.
#
#   Before calling this function, you can define VTK_PYTHON_EXE to point the
#   executable (or generator expression for the executable) to run for the test.
#   If VTK_PYTHON_EXE is not defined, `vtkpython` is assumed i.e.
#   ($<TARGET_FILE:vtkpython>)
function(vtk_add_test_python)
  if (NOT DEFINED VTK_PYTHON_EXE)
    set(VTK_PYTHON_EXE "\$<TARGET_FILE:vtkpython>")
  endif()
  set(python_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    NO_RT
    JUST_VALID
    )
  _vtk_test_parse_args("${python_options}" "py" ${ARGN})
  _vtk_test_set_options("${python_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  set(data_dir "${VTK_TEST_DATA_DIR}")
  if(${vtk-module}_DATA_DIR)
    set(data_dir "${${vtk-module}_DATA_DIR}")
  endif()

  set(baseline_dir ${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline)
  if(VTK_BASELINE_DIR)
    set(baseline_dir ${VTK_BASELINE_DIR})
  endif()

  set(externaldata_target VTKData)
  if(VTK_TEST_DATA_TARGET)
    set(externaldata_target ${VTK_TEST_DATA_TARGET})
  endif()

  foreach(name IN LISTS names)
    _vtk_test_set_options("${python_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    set(_D "")
    if(NOT local_NO_DATA)
      set(_D -D ${data_dir})
    endif()

    set(rtImageTest "")
    set(_B "")
    set(_V "")
    set(_A "")
    if(NOT local_NO_VALID)
      if(local_NO_RT)
        set(_B -B "DATA{${baseline_dir}/,REGEX:${test_name}(_[0-9]+)?.png}")
      else()
        set(_V -V "DATA{${baseline_dir}/${test_name}.png,:}")
        if(NOT local_JUST_VALID)
          set(rtImageTest ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py)
          set(_A -A ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py)
        endif()
      endif()
    endif()

    set(_T "")
    if(NOT local_NO_OUTPUT)
      set(_T -T ${VTK_TEST_OUTPUT_DIR})
    endif()

    ExternalData_add_test(${externaldata_target}
      NAME    ${vtk-module}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}
      COMMAND ${_vtk_test_python_pre_args}
              ${VTK_PYTHON_EXE} --enable-bt
              ${VTK_PYTHON_ARGS}
              ${rtImageTest}
              ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}.py
              ${args}
              ${${vtk-module}_ARGS}
              ${${name}_ARGS}
              ${_D} ${_B} ${_T} ${_V} ${_A})
    set_tests_properties(${vtk-module}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}
      PROPERTIES
        LABELS "${${vtk-module}_TEST_LABELS}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        # This must match the skip() function in vtk/test/Testing.py"
        SKIP_RETURN_CODE 125
      )
  endforeach()
endfunction()

#   Before calling this function, you can define VTK_PYTHON_EXE to point the
#   executable (or generator expression for the executable) to run for the test.
#   If VTK_PYTHON_EXE is not defined, `vtkpython` is assumed i.e.
#   ($<TARGET_FILE:pvtkpython>)
function(vtk_add_test_python_mpi)
  set(_vtk_test_python_suffix "-MPI")

  set(numprocs ${VTK_MPI_MAX_NUMPROCS})
  if(${vtk-module}_NUMPROCS)
    set(numprocs ${${vtk-module}_NUMPROCS})
  endif()

  set(_vtk_test_python_pre_args
    ${VTK_MPIRUN_EXE}
    ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${numprocs}
    ${VTK_MPI_PREFLAGS})

  if (NOT DEFINED VTK_PYTHON_EXE)
    set(VTK_PYTHON_EXE "\$<TARGET_FILE:pvtkpython>")
  endif()
  vtk_add_test_python(${ARGN})
endfunction()

# -----------------------------------------------------------------------------
# vtk_add_test_tcl([NO_RT] [NO_DATA] [NO_VALID] [NO_OUTPUT]
#                  [test1.tcl...] [args...])
#   Adds Tcl tests.
#
#   Adds Tcl tests to run. If NO_DATA is set, the -D argument to the test
#   (input data) will not be passed, NO_VALID will suppress -V, NO_OUTPUT will
#   suppress -T, and NO_RT will suppress the -V and -T arguments
#   unconditionally and pass -D to the empty string. Test-specific arguments may
#   be set to _${name}_ARGS. By default, the test name will be the part of the
#   source file before the '.tcl'. A custom test name can be specified by giving
#   a name followed by a comma before the test file name, .e.g.,
#
#   CustomTestName,TestSource.tcl
#
#   The 'vtk_test_prefix' variable may be set to create separate tests from a
#   single test name (e.g., running with different arguments), but should be
#   used only when required.
function(vtk_add_test_tcl)
  if(NOT VTK_TCL_EXE)
    message(FATAL_ERROR "VTK_TCL_EXE not set")
  endif()
  set(tcl_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    NO_RT
    )
  _vtk_test_parse_args("${tcl_options}" "tcl" ${ARGN})
  _vtk_test_set_options("${tcl_options}" "" ${options})

  set(_vtk_fail_regex "(\n|^)ERROR: " "instance(s)? still around")

  set(data_dir "${VTK_TEST_DATA_DIR}")
  if(${vtk-module}_DATA_DIR)
    set(data_dir "${${vtk-module}_DATA_DIR}")
  endif()

  set(baseline_dir ${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline)
  if(VTK_BASELINE_DIR)
    set(baseline_dir ${VTK_BASELINE_DIR})
  endif()

  set(externaldata_target VTKData)
  if(VTK_TEST_DATA_TARGET)
    set(externaldata_target ${VTK_TEST_DATA_TARGET})
  endif()

  foreach(name IN LISTS names)
    _vtk_test_set_options("${tcl_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name})

    if(NOT local_NO_DATA)
      set(_D -D ${data_dir})
    elseif(local_NO_RT)
      set(_D "")
    else()
      set(_D -D VTK_DATA_ROOT-NOTFOUND)
    endif()

    set(rtImageTest "")
    set(_V "")
    set(_T "")
    if(NOT local_NO_RT)
      set(rtImageTest ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl)
      if(NOT local_NO_VALID)
        set(_V -V "DATA{${baseline_dir}/${test_name}.png,:}")
      endif()
      if(NOT local_NO_OUTPUT)
        set(_T -T ${VTK_TEST_OUTPUT_DIR})
      endif()
    endif()
    set(_A -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

    ExternalData_add_test(${externaldata_target}
      NAME    ${vtk-module}Tcl-${vtk_test_prefix}${test_name}
      COMMAND ${VTK_TCL_EXE}
              ${rtImageTest}
              ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}.tcl
              ${args}
              ${${vtk-module}_ARGS}
              ${${name}_ARGS}
              ${_D} ${_T} ${_V} ${_A})
    set_tests_properties(${vtk-module}Tcl-${vtk_test_prefix}${test_name}
      PROPERTIES
        LABELS "${${vtk-module}_TEST_LABELS}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
      )
  endforeach()

endfunction()
