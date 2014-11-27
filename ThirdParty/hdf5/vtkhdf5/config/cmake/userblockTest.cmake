# runTest.cmake executes a command and captures the output in a file. File is then compared
# against a reference file. Exit status of command can also be compared.

# arguments checking
if (NOT TEST_PROGRAM)
  message (FATAL_ERROR "Require TEST_PROGRAM tellub to be defined")
endif (NOT TEST_PROGRAM)
if (NOT TEST_GET_PROGRAM)
  message (FATAL_ERROR "Require TEST_GET_PROGRAM getub to be defined")
endif (NOT TEST_GET_PROGRAM)
if (NOT TEST_FOLDER)
  message ( FATAL_ERROR "Require TEST_FOLDER to be defined")
endif (NOT TEST_FOLDER)
if (NOT TEST_HFILE)
  message (FATAL_ERROR "Require TEST_HFILE the hdf file to be defined")
endif (NOT TEST_HFILE)
if (NOT TEST_UFILE)
  message (FATAL_ERROR "Require TEST_UFILE the ub file to be defined")
endif (NOT TEST_UFILE)
if (NOT TEST_CHECKUB)
  message (STATUS "Require TEST_CHECKUB - YES or NO - to be defined")
endif (NOT TEST_CHECKUB)
#if (NOT TEST_EXPECT)
#  message (STATUS "Require TEST_EXPECT to be defined")
#endif (NOT TEST_EXPECT)
#if (NOT TEST_OFILE)
#  message (FATAL_ERROR "Require TEST_OFILE the original hdf file to be defined")
#endif (NOT TEST_OFILE)

set (TEST_U_STRING_LEN 0)
set (TEST_O_STRING_LEN 0)
set (TEST_H_STRING_LEN 0)
set (TEST_STRING_SIZE 0)

if (TEST_CHECKUB STREQUAL "YES")
  # find the length of the user block to check
  #s1=`cat $ufile | wc -c | sed -e 's/ //g'`
  file (STRINGS ${TEST_FOLDER}/${TEST_UFILE} TEST_U_STRING)
  string (LENGTH ${TEST_U_STRING} TEST_U_STRING_LEN)

  # Get the size of the original user block, if any.
  if (TEST_OFILE)
    # 'tellub' calls H5Fget_user_block to get the size
    #  of the user block
    #s2=`$JAM_BIN/tellub $origfile`
    EXECUTE_PROCESS (
        COMMAND ${TEST_PROGRAM} ${TEST_OFILE}
        WORKING_DIRECTORY ${TEST_FOLDER}
        RESULT_VARIABLE TEST_RESULT
        OUTPUT_FILE ${TEST_HFILE}.len.txt
        OUTPUT_VARIABLE TEST_ERROR
        ERROR_VARIABLE TEST_ERROR
    )
    if (NOT ${TEST_RESULT} STREQUAL "0")
      message (FATAL_ERROR "Failed: The output of ${TEST_PROGRAM} ${TEST_OFILE} is: ${TEST_ERROR}")
    endif (NOT ${TEST_RESULT} STREQUAL "0")
    file (READ ${TEST_HFILE}.len.txt TEST_O_STRING_LEN)
  endif (TEST_OFILE)
   
  MATH( EXPR TEST_STRING_SIZE "${TEST_U_STRING_LEN} + ${TEST_O_STRING_LEN}" )
 
  if (NOT TEST_O_STRING_LEN STREQUAL "0")
    #$JAM_BIN/getub -c $s2 $origfile > $cmpfile
    EXECUTE_PROCESS (
        COMMAND ${TEST_GET_PROGRAM} -c ${TEST_O_STRING_LEN} ${TEST_OFILE}
        WORKING_DIRECTORY ${TEST_FOLDER}
        RESULT_VARIABLE TEST_RESULT
        OUTPUT_FILE ${TEST_HFILE}-ub.cmp
        OUTPUT_VARIABLE TEST_ERROR
        ERROR_VARIABLE TEST_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    #cat $ufile >> $cmpfile
    file (STRINGS ${TEST_UFILE} TEST_STREAM NEWLINE_CONSUME)
    file (APPEND ${TEST_HFILE}-ub.cmp "${TEST_STREAM}") 
  else (NOT TEST_O_STRING_LEN STREQUAL "0")
    file (STRINGS ${TEST_UFILE} TEST_STREAM NEWLINE_CONSUME)
    file (WRITE ${TEST_HFILE}-ub.cmp ${TEST_STREAM})
  endif (NOT TEST_O_STRING_LEN STREQUAL "0")

  #$JAM_BIN/getub -c $size $hfile > $tfile
  EXECUTE_PROCESS (
      COMMAND ${TEST_GET_PROGRAM} -c ${TEST_STRING_SIZE} ${TEST_HFILE}
      WORKING_DIRECTORY ${TEST_FOLDER}
      RESULT_VARIABLE TEST_RESULT
      OUTPUT_FILE ${TEST_HFILE}.cmp
      OUTPUT_VARIABLE TEST_ERROR
      ERROR_VARIABLE TEST_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # now compare the outputs
  EXECUTE_PROCESS (
      COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_HFILE}-ub.cmp ${TEST_HFILE}.cmp
      RESULT_VARIABLE TEST_RESULT
  )

  message (STATUS "COMPARE Result: ${TEST_RESULT}: ${TEST_STRING_SIZE}=${TEST_U_STRING_LEN}+${TEST_O_STRING_LEN}")
  # if the return value is !=${TEST_EXPECT} bail out
  if (NOT ${TEST_RESULT} STREQUAL ${TEST_EXPECT})
    message (FATAL_ERROR "Failed: The output of ${TEST_HFILE}-ub did not match ${TEST_HFILE}.\n${TEST_ERROR}")
  endif (NOT ${TEST_RESULT} STREQUAL ${TEST_EXPECT})
else (TEST_CHECKUB STREQUAL "YES")
    # call 'ubsize' to get the size of the user block
    #ubsize=`$JAM_BIN/tellub $hfile`
    EXECUTE_PROCESS (
        COMMAND ${TEST_PROGRAM} ${TEST_HFILE}
        WORKING_DIRECTORY ${TEST_FOLDER}
        RESULT_VARIABLE TEST_H_STRING_LEN
        OUTPUT_VARIABLE TEST_ERROR
        ERROR_VARIABLE TEST_ERROR
    )
  if (NOT TEST_H_STRING_LEN STREQUAL "0")
    message (FATAL_ERROR "Failed: The output of ${TEST_HFILE} was NOT empty")
  endif (NOT TEST_H_STRING_LEN STREQUAL "0")
endif (TEST_CHECKUB STREQUAL "YES")

# everything went fine...
message ("Passed: The output of CHECK matched expectation")

