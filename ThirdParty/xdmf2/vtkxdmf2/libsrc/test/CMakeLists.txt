# In CMake version 2.8 we can used a much nicer $<CONFIGURATION> arguments for specifying 
# the configuration type of the build For now I have added this new functionality to these 
# batch tests but it will only work on dashboards using 2.8. This check can be removed when
# ParaView officially requires CMake 2.8."
SET(GENERATOR_EXPRESSIONS_SUPPORTED)
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)
  SET(GENERATOR_EXPRESSIONS_SUPPORTED OFF)
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)
  SET(GENERATOR_EXPRESSIONS_SUPPORTED ON)
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)

SET(TEST_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH})

# We don't want the <CONFIGURATION> as part of the path on non windows systems.
IF(WIN32 AND GENERATOR_EXPRESSIONS_SUPPORTED)
  SET(TEST_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH}/\$<CONFIGURATION>)
ENDIF(WIN32 AND GENERATOR_EXPRESSIONS_SUPPORTED)

macro( XdmfSerialTest testname )
    add_executable( ${testname} ${ARGN} )
    target_link_libraries( ${testname} Xdmf )
    add_test( ${testname} ${EXECUTABLE_OUTPUT_PATH}/${testname} )
endmacro()

macro( XdmfMPITest testname num_procs )
    find_package( MPI )
    if( MPI_FOUND AND GENERATOR_EXPRESSIONS_SUPPORTED)
        include_directories( ${MPI_INCLUDE_PATH} )
        add_executable( ${testname} ${ARGN} )
        target_link_libraries( ${testname} Xdmf ${MPI_LIBRARIES} )
        add_test(NAME ${testname} 
          COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${num_procs}
            ${MPIEXEC_PREFLAGS}
            ${TEST_EXECUTABLE_PATH}/${testname}
            ${MPI_EXEC_POSTFLAGS} )
    endif( MPI_FOUND AND GENERATOR_EXPRESSIONS_SUPPORTED)
endmacro( XdmfMPITest )

macro( XdmfUtilsTest testname )
    add_executable (${testname} ${ARGN} )
    target_link_libraries( ${testname} XdmfUtils )
    add_test( ${testname} ${EXECUTABLE_OUTPUT_PATH}/${testname} )
endmacro()

XdmfSerialTest( XdmfTestHDFRoundtrip TestXdmfHDF.cxx )
#XdmfSerialTest( XdmfTestArray TestXdmfArray.cc)

if(XDMF_BUILD_MPI)
    XdmfMPITest( XdmfTestHDFMPI 4 XdmfHDFMPI.cxx )
endif(XDMF_BUILD_MPI)

if(XDMF_WRAP_PYTHON)
  if(PYTHON_EXECUTABLE)
    CONFIGURE_FILE(PythonTestDriver.cmake.in PythonTestDriver.cmake @ONLY)
    SET(PYTHON_TESTS
        TestXdmfArrayPython
        TestXdmfWriteGridsToFilePython
       )
    FOREACH(PYTHON_TEST ${PYTHON_TESTS})
      CONFIGURE_FILE(${PYTHON_TEST}.py ${PYTHON_TEST}.py COPYONLY)
      # Run all python tests by calling a cmake script that sets correct
      # environment variables --- python script to run and appropraite arguments
      # are set in ARGV and passed to the cmake script
      SET(ARGV ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_TEST}.py)
      ADD_TEST("${PYTHON_TEST}" ${CMAKE_COMMAND} "-D" ARGV=${ARGV} "-P" ${CMAKE_CURRENT_BINARY_DIR}/PythonTestDriver.cmake) 
    ENDFOREACH(PYTHON_TEST)
  endif(PYTHON_EXECUTABLE)
endif(XDMF_WRAP_PYTHON)
