# vfdTest.cmake executes a command and captures the output in a file. Command uses specified VFD.
# Exit status of command can also be compared.

# arguments checking
if (NOT TEST_PROGRAM)
  message (FATAL_ERROR "Require TEST_PROGRAM to be defined")
endif (NOT TEST_PROGRAM)
#if (NOT TEST_ARGS)
#  message (STATUS "Require TEST_ARGS to be defined")
#endif (NOT TEST_ARGS)
#if (NOT TEST_EXPECT)
#  message (STATUS "Require TEST_EXPECT to be defined")
#endif (NOT TEST_EXPECT)
if (NOT TEST_FOLDER)
  message ( FATAL_ERROR "Require TEST_FOLDER to be defined")
endif (NOT TEST_FOLDER)
if (NOT TEST_VFD)
  message (FATAL_ERROR "Require TEST_VFD to be defined")
endif (NOT TEST_VFD)

set (ERROR_APPEND 1)

message (STATUS "USING ${TEST_VFD} ON COMMAND: ${TEST_PROGRAM} ${TEST_ARGS}")

set (ENV{HDF5_DRIVER} "${TEST_VFD}")
# run the test program, capture the stdout/stderr and the result var
EXECUTE_PROCESS (
    COMMAND ${TEST_PROGRAM} ${TEST_ARGS}
    WORKING_DIRECTORY ${TEST_FOLDER}
    OUTPUT_FILE ${TEST_OUTPUT}_${TEST_VFD}.out
    ERROR_FILE ${TEST_OUTPUT}_${TEST_VFD}.err
    OUTPUT_VARIABLE TEST_ERROR
    ERROR_VARIABLE TEST_ERROR
)

message (STATUS "COMMAND Result: ${TEST_RESULT}")

if (ERROR_APPEND)
  file (READ ${TEST_FOLDER}/${TEST_OUTPUT}_${TEST_VFD}.err TEST_STREAM)
  file (APPEND ${TEST_FOLDER}/${TEST_OUTPUT}_${TEST_VFD}.out "${TEST_STREAM}") 
endif (ERROR_APPEND)

# if the return value is !=${TEST_EXPECT} bail out
if (NOT ${TEST_RESULT} STREQUAL ${TEST_EXPECT})
  message ( FATAL_ERROR "Failed: Test program ${TEST_PROGRAM} exited != ${TEST_EXPECT}.\n${TEST_ERROR}")
endif (NOT ${TEST_RESULT} STREQUAL ${TEST_EXPECT})

# everything went fine...
message ("Passed: The ${TEST_PROGRAM} program used vfd ${TEST_VFD}")

