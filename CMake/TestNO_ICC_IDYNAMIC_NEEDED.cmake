# Tests whether the compiler is Intel icc and needs -i_dynamic

MACRO(TESTNO_ICC_IDYNAMIC_NEEDED VARIABLE LOCAL_TEST_DIR)
  IF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
    TRY_RUN(${VARIABLE} HAVE_${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${LOCAL_TEST_DIR}/TestNO_ICC_IDYNAMIC_NEEDED.cxx
      OUTPUT_VARIABLE OUTPUT)
    MESSAGE(STATUS "Check if using the Intel icc compiler, and if -i_dynamic is needed... COMPILE_RESULT...${HAVE_${VARIABLE}} RUN_RESULT...${VARIABLE}\n")
    IF(HAVE_${VARIABLE}) #Test compiled, either working intel w/o -i_dynamic, or another compiler
      IF(${VARIABLE})   #Intel icc compiler, -i_dynamic not needed
        FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
                       "-i_dynamic not needed, (Not Intel icc, or this version of Intel icc does not conflict with OS glibc.")
        MESSAGE(STATUS "-i_dynamic not needed, (Not Intel icc, or this version of Intel icc does not conflict with OS glibc.")
      ELSE(${VARIABLE}) #The compiler is not Intel icc
        FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log 
                       "The compiler ERROR--This should never happen")
        MESSAGE(STATUS "The compiler ERROR--This should never happen")
      ENDIF(${VARIABLE})
    ELSE(HAVE_${VARIABLE})  #Test did not compile, either badly broken compiler, or intel -i_dynamic needed
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
            "\tThe -i_dynamic compiler flag is needed for the Intel icc compiler on this platform.\n")
      MESSAGE("The -i_dynamic compiler flag is needed for the Intel icc compiler on this platform.")
    ENDIF(HAVE_${VARIABLE})
    FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log "TestNO_ICC_IDYNAMIC_NEEDED produced following output:\n${OUTPUT}\n\n")
  ENDIF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
ENDMACRO(TESTNO_ICC_IDYNAMIC_NEEDED)
