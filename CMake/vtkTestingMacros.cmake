#-----------------------------------------------------------------------------
# Private helper macros.

macro(parse_optional_arguments)
  set (BASELINEDIR ${vtk-module})
  #set (DATADIR ${VTK_DATA_ROOT}) #don't do this: some tests don't want it

  set(argv ${ARGV})
  set(MYARGV)
  set (i 0)
  while (${i} LESS ${ARGC})
    math(EXPR iplus1 "${i}+1")
    list(GET argv ${i} ARG)
    if (${ARG} STREQUAL "BASELINEDIR")
      list(GET argv ${iplus1} BASELINEDIR)
      set (i ${iplus1})
      math(EXPR iplus1 "${i}+1")
    elseif(${ARG} STREQUAL "DATADIR")
      list(GET argv ${iplus1} DATADIR)
      set (i ${iplus1})
      math(EXPR iplus1 "${i}+1")
    elseif(${ARG} STREQUAL "LABELS" AND ${iplus1} LESS ${ARGC})
      # everything after LABELS gets added as a label
      set(LABELS)
      while (${iplus1} LESS ${ARGC})
        list(GET argv ${iplus1} LABEL)
        list(APPEND LABELS ${LABEL})
        set(i ${iplus1})
        math(EXPR iplus1 "${i}+1")
      endwhile()
    else()
      list(APPEND MYARGV ${ARG})
    endif()
    set(i ${iplus1})
  endwhile()
endmacro(parse_optional_arguments)

#-----------------------------------------------------------------------------
# Public interface macros.


# -----------------------------------------------------------------------------
# vtk_tests(cxxfiles [BASELINEDIR baseline_directory] [DATADIR data_directory] [LABELS test_labels])
#
# Takes a list of cxx files which will be driven by the modules
# test driver. This helps reduce a lot of boiler place code in each module
#
# BASELINEDIR a baseline directory to look for correct images in. If not
# specified it will look for a directory named for the module the test is in.
#
# DATADIR a data directory to look for input data to the tests in. If not
# specified the test is assumed to not require input data.
# Ex. ${VTK_DATA_ROOT} or ${VTK_LARGE_DATA_ROOT}
#
# LABELS labels to be added to the tests. Note that the
# [LABELS test_labels] must be at the end of the macro call since all strings
# after LABELS will be added as labels to the tests.
# Ex. PARAVIEW to label that the test is for ParaView and can be run
# with ctest -L PARAVIEW
macro(vtk_tests)

  parse_optional_arguments(${ARGV})
  create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${MYARGV}
    EXTRA_INCLUDE vtkTestDriver.h)

  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    if(DATADIR)
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        -D ${DATADIR}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${BASELINEDIR}/${TName}.png)
    else()
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
        ${${TName}_ARGS})
    endif()
    if(LABELS)
      set_tests_properties(${vtk-module}Cxx-${TName} PROPERTIES LABELS "${LABELS}")
    endif()
  endforeach()
endmacro(vtk_tests)

# -----------------------------------------------------------------------------
# vtk_add_test_mpi(filenames [DATADIR data_directory)]
# Adds one or more tests that are run under MPI.
#
# DATADIR a data directory to look for input data to the tests in. If not
# specified the test is assumed to not require input data.
# Ex. ${VTK_DATA_ROOT} or ${VTK_LARGE_DATA_ROOT}
macro (vtk_add_test_mpi fileName)

  parse_optional_arguments(${ARGV})

  get_filename_component(name ${fileName} NAME_WE)
  list(REMOVE_AT MYARGV 0)
  vtk_module_test_executable(
    ${name}
    ${name}.cxx
    ${MYARGV})

  if(DATADIR)
    add_test(
      NAME ${vtk-module}Cxx-MPI-${name}
      COMMAND ${VTK_MPIRUN_EXE}
      ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
      ${VTK_MPI_PREFLAGS}
      $<TARGET_FILE:${name}>
      -D ${DATADIR}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      -V ${DATADIR}/Baseline/Parallel/${name}.png
      ${VTK_MPI_POSTFLAGS})
  else()
    add_test(
      NAME ${vtk-module}Cxx-MPI-${name}
      COMMAND ${VTK_MPIRUN_EXE}
      ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
      ${VTK_MPI_PREFLAGS}
      $<TARGET_FILE:${name}>
      ${VTK_MPI_POSTFLAGS})
  endif()
endmacro()

# -----------------------------------------------------------------------------
# Usage: vtk_add_test_cxx([name.cxx[,-E<n>][,NO_VALID]]...
#          [NO_DATA] [NO_VALID|<base_dir>] [VALID_ERROR <n>])
function(vtk_add_test_cxx)
  # Parse Command line args
  set(names "")
  set(no_data 0)
  set(no_valid 0)
  set(no_output 0)
  set(large_data 0)
  unset(base_dir)
  foreach(a IN LISTS ARGN)
    if("[${a}]" STREQUAL "[NO_DATA]")
      set(no_data 1)
    elseif("[${a}]" STREQUAL "[NO_VALID]")
      set(no_valid 1)
    elseif("[${a}]" STREQUAL "[NO_OUTPUT]")
      set(no_output 1)
    elseif("[${a}]" STREQUAL "[LARGE_DATA]")
      set(large_data 1)
    elseif("x${a}" MATCHES "^x([^.]*)\\.cxx,?(.*)$")
      set(name "${CMAKE_MATCH_1}")
      string(REPLACE "," ";" _${name}_OPTIONS "${CMAKE_MATCH_2}")
      list(APPEND names ${name})
    elseif(NOT DEFINED base_dir)
      set(base_dir "${a}")
    else()
      message(FATAL_ERROR "Unknown argument \"${a}\"")
    endif()
  endforeach()

  if(no_valid)
    set(base_dir "")
  elseif(NOT DEFINED base_dir)
    message(FATAL_ERROR "Call must specify either NO_VALID or <base_dir> ")
  endif()

  if(large_data)
    set(data_dir "${VTK_LARGE_DATA_ROOT}")
  else()
    set(data_dir "${VTK_DATA_ROOT}")
  endif()

  if(data_dir AND NOT no_data)
    set(_D -D ${data_dir})
  else()
    set(_D "")
  endif()

  set(_T "")
  if(NOT no_output)
    set(_T -T ${VTK_TEST_OUTPUT_DIR})
  endif()

  foreach(name ${names})
    set(_V "")
    set(_E "")
    set(tmp_base "${base_dir}")
    foreach(opt IN LISTS _${name}_OPTIONS)
      if("x${opt}" MATCHES "^x-E([0-9]+)$")
        set(_E -E ${CMAKE_MATCH_1})
      elseif("[${opt}]" STREQUAL "[NO_VALID]")
        set(tmp_base "")
      else()
        message(FATAL_ERROR "Test ${name} has unknown option \"${opt}\"")
      endif()
    endforeach()
    if(data_dir AND tmp_base)
      set(_V -V Baseline/${tmp_base}/${name}.png)
    endif()
    add_test(NAME ${vtk-module}Cxx-${name}
      COMMAND ${vtk-module}CxxTests ${name} ${${name}_ARGS}
      ${_D} ${_T} ${_V} ${_E})
    set_property(DIRECTORY APPEND PROPERTY VTK_TEST_CXX_SOURCES ${name}.cxx)
  endforeach()
endfunction()

macro(vtk_test_cxx_executable exe_name)
  set(argn "${ARGN}")
  set(test_driver vtkTestDriver.h)
  set(test_extra "")
  foreach(a IN LISTS argn)
    if("[${a}]" STREQUAL "[RENDERING_FACTORY]")
      include(vtkTestingRenderingDriver)
      set(test_driver ${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.h)
    elseif("x${a}" MATCHES "\\.cxx$")
      list(APPEND test_extra ${a})
    else()
      message(FATAL_ERROR "Unknown argument \"${a}\"")
    endif()
  endforeach()
  get_property(vtk_test_cxx_sources DIRECTORY PROPERTY VTK_TEST_CXX_SOURCES)
  create_test_sourcelist(Tests ${exe_name}.cxx ${vtk_test_cxx_sources}
    EXTRA_INCLUDE ${test_driver})
  vtk_module_test_executable(${exe_name} ${Tests} ${test_extra})
endmacro()

# -----------------------------------------------------------------------------
# Usage: vtk_add_test_python(name [NO_RT] [NO_DATA] [NO_VALID|<base_dir>])
# NO_RT is for tests using vtk.test.testing
function(vtk_add_test_python name)
  if(NOT VTK_PYTHON_EXE)
    message(FATAL_ERROR "VTK_PYTHON_EXE not set")
  endif()
  # Parse Command line args
  get_filename_component(TName ${name} NAME_WE)
  set(no_data 0)
  set(no_valid 0)
  set(no_output 0)
  set(no_rt 0)
  unset(base_dir)
  foreach(a IN LISTS ARGN)
    if("[${a}]" STREQUAL "[NO_DATA]")
      set(no_data 1)
    elseif("[${a}]" STREQUAL "[NO_VALID]")
      set(no_valid 1)
    elseif("[${a}]" STREQUAL "[NO_OUTPUT]")
      set(no_output 1)
    elseif("[${a}]" STREQUAL "[NO_RT]")
      set(no_rt 1)
    elseif(NOT DEFINED base_dir)
      set(base_dir "${a}")
    else()
      message(FATAL_ERROR "Unknown argument \"${a}\"")
    endif()
  endforeach()

  if(no_valid)
    set(base_dir "")
  elseif(NOT DEFINED base_dir)
    message(FATAL_ERROR "Call must specify either NO_VALID or <base_dir> ")
  endif()

  if(VTK_DATA_ROOT AND NOT no_data)
    set(_D -D ${VTK_DATA_ROOT})
  else()
    set(_D "")
  endif()

  set(rtImageTest "")
  set(_B "")
  set(_V "")
  set(_T "")
  set(_A "")
  if(VTK_DATA_ROOT AND base_dir)
    if(no_rt)
      set(_B -B ${VTK_DATA_ROOT}/Baseline/${base_dir})
    else()
      set(rtImageTest ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py)
      set(_V -V Baseline/${base_dir}/${TName}.png)
      set(_A -A ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py)
    endif()
    if(NOT no_output)
      set(_T -T ${VTK_TEST_OUTPUT_DIR})
    endif()
  endif()

  add_test(NAME ${vtk-module}Python-${TName}
    COMMAND ${VTK_PYTHON_EXE} ${rtImageTest}
    ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.py ${${TName}_ARGS}
    ${_D} ${_B} ${_T} ${_V} ${_A})
endfunction()

# -----------------------------------------------------------------------------
# Usage: vtk_add_test_tcl(name [NO_DATA] [NO_VALID|<base_dir>])
function(vtk_add_test_tcl name)
  if(NOT VTK_TCL_EXE)
    message(FATAL_ERROR "VTK_TCL_EXE not set")
  endif()
  # Parse Command line args
  get_filename_component(TName ${name} NAME_WE)
  set(no_data 0)
  set(no_valid 0)
  set(no_output 0)
  set(no_rt 0)
  unset(base_dir)
  foreach(a IN LISTS ARGN)
    if("[${a}]" STREQUAL "[NO_DATA]")
      set(no_data 1)
    elseif("[${a}]" STREQUAL "[NO_VALID]")
      set(no_valid 1)
    elseif("[${a}]" STREQUAL "[NO_OUTPUT]")
      set(no_output 1)
    elseif("[${a}]" STREQUAL "[NO_RT]")
      set(no_rt 1)
    elseif(NOT DEFINED base_dir)
      set(base_dir "${a}")
    else()
      message(FATAL_ERROR "Unknown argument \"${a}\"")
    endif()
  endforeach()

  if(no_valid OR no_rt)
    set(base_dir "")
  elseif(NOT DEFINED base_dir)
    message(FATAL_ERROR "Call must specify either NO_VALID or <base_dir> ")
  endif()

  if(VTK_DATA_ROOT AND NOT no_data)
    set(_D -D ${VTK_DATA_ROOT})
  elseif(no_rt)
    set(_D "")
  else()
    set(_D -D VTK_DATA_ROOT-NOTFOUND)
  endif()

  set(rtImageTest "")
  set(_V "")
  set(_T "")
  if(NOT no_rt)
    set(rtImageTest ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl)
    if(VTK_DATA_ROOT AND base_dir)
      set(_V -V Baseline/${base_dir}/${TName}.png)
    endif()
    if(NOT no_output)
      set(_T -T ${VTK_TEST_OUTPUT_DIR})
    endif()
  endif()
  set(_A -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

  add_test(NAME ${vtk-module}Tcl-${TName}
    COMMAND ${VTK_TCL_EXE} ${rtImageTest}
    ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.tcl ${${TName}_ARGS}
    ${_D} ${_T} ${_V} ${_A})
endfunction()
