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
# add_test_mpi(filenames [DATADIR data_directory)]
# Adds one or more tests that are run under MPI.
#
# DATADIR a data directory to look for input data to the tests in. If not
# specified the test is assumed to not require input data.
# Ex. ${VTK_DATA_ROOT} or ${VTK_LARGE_DATA_ROOT}
macro (add_test_mpi fileName)

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
# vtk_tests_python() macro takes a list of python files and makes them into
# proper python tests.
macro(vtk_tests_python)
  if(VTK_PYTHON_EXE)
    foreach(test ${ARGV})
      get_filename_component(TName ${test} NAME_WE)
      if(VTK_DATA_ROOT)
        string (REPLACE "vtk" "" _baselinedname ${vtk-module})
        add_test(NAME ${vtk-module}Python-${TName}
          COMMAND ${VTK_PYTHON_EXE}
          ${CMAKE_CURRENT_SOURCE_DIR}/${test}
          -D ${VTK_DATA_ROOT}
          -B ${VTK_DATA_ROOT}/Baseline/${_baselinedname})
      else()
        add_test(NAME ${vtk-module}Python-${TName}
          COMMAND ${VTK_PYTHON_EXE}
          ${CMAKE_CURRENT_SOURCE_DIR}/${test}
          ${${TName}_ARGS})
      endif()
    endforeach()
  else()
    messge(FATAL "VTK_PYTHON_EXE not set")
  endif()

endmacro(vtk_tests_python)

# -----------------------------------------------------------------------------
# add_test_python() macro takes a python file and an optional base directory where the
# corresponding test image is found and list of python files and makes them into
# proper python tests.
macro(add_test_python)
  if(VTK_PYTHON_EXE)
    # Parse Command line args
    get_filename_component(TName ${ARGV0} NAME_WE)
    string (REPLACE "vtk" "" _baselinedname ${vtk-module})
    # Check if data root and second parameter is present
    if(VTK_DATA_ROOT AND NOT ARGV1)
      add_test(NAME ${vtk-module}Python-${TName}
        COMMAND ${VTK_PYTHON_EXE}
        ${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py/rtImageTest.py
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.py
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${ARGV1}/${TName}.png
        -A "${VTK_BINARY_DIR}/Utilities/vtkTclTest2Py"
        -A "${VTK_LIBRARY_DIR}")
    else()
      add_test(NAME ${vtk-module}Python-${TName}
        COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.py
        ${${TName}_ARGS})
    endif()
  else()
    messge(FATAL "VTK_PYTHON_EXE not set")
  endif()
endmacro(add_test_python)

# -----------------------------------------------------------------------------
# This macro is for tests using vtk.test.testing
# add_test_python1() macro takes a python file and an optional base directory where the
# corresponding test image is found and list of python files and makes them into
# proper python tests.
# Usage: add_test_python1(name base_dir)
#    Where: name - the name of the test
#                  e.g x.py
#           base_dir - the (optional) base directory where the test image is
#                      e.g Baseline/Graphics
macro(add_test_python1)
  if(VTK_PYTHON_EXE)
    # Parse Command line args
    get_filename_component(TName ${ARGV0} NAME_WE)
    string (REPLACE "vtk" "" _baselinedname ${vtk-module})
    # Check if data root and second parameter is present
    if(VTK_DATA_ROOT AND NOT ARGV1)
      add_test(NAME ${vtk-module}Python-${TName}
        COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.py
        -D ${VTK_DATA_ROOT}
        -B ${VTK_DATA_ROOT}/${ARGV1}
        -T "${VTK_BINARY_DIR}/Testing/Temporary")
    else()
      add_test(NAME ${vtk-module}Python-${TName}
        COMMAND ${VTK_PYTHON_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.py
        ${${TName}_ARGS})
    endif()
  else()
    messge(FATAL "VTK_PYTHON_EXE not set")
  endif()
endmacro(add_test_python1)

# -----------------------------------------------------------------------------
# add_test_tcl() macro takes a tcl file and an optional base directory where the
# corresponding test image is found and list of tcl files and makes them into
# proper tcl tests.
macro(add_test_tcl)
  if(VTK_TCL_EXE)
    # Parse Command line args
    get_filename_component(TName ${ARGV0} NAME_WE)
    string (REPLACE "vtk" "" _baselinedname ${vtk-module})
    # Check if data root and second parameter is present
    if(VTK_DATA_ROOT AND NOT ARGV1)
      add_test(NAME ${vtk-module}Tcl-${TName}
        COMMAND ${VTK_TCL_EXE}
        ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${ARGV1}/${TName}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    else()
      add_test(NAME ${vtk-module}Tcl-${TName}
        COMMAND ${VTK_TCL_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${TName}.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
        ${${TName}_ARGS})
    endif()
  else()
    messge(FATAL "VTK_TCL_EXE not set")
  endif()
endmacro(add_test_tcl)
