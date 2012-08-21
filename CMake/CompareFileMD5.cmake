#
# Simple CMake -P script to compare the MD5 hash of files. Usage:
#
# cmake -DTESTFILE=<filespec> -DGLOBEXP=<glob expression> -DCLEANTESTFILE=<bool> -P CompareFileMD5.cmake
#
# TESTFILE is the test file, GLOB expression referring to a collection of
# reference files. This can be used to compare file produced by a test
# (TESTFILE) to one or more baselines (glob expression GLOBEXP). If
# CLEANTESTFILE is set to some true value, the test file will be deleted if a
# matching hash is found.
#
# The script will exit silently if the test file's hash matches the hash
# of any reference file (success), or print a line starting with MATCH_FAIL
# and several lines of supplemental information otherwise.
#

macro(dumpoutput lines)
  foreach(line ${lines})
    message("MATCH_FAIL: ${line}")
  endforeach()
endmacro()

set(HASH_FOUND FALSE)

set(output "Comparing MD5 sum of;  \"${TESTFILE}\";  to glob;  \"${GLOBEXP}\"")

file(GLOB REFFILES ${GLOBEXP})

if(NOT REFFILES)
  list(APPEND output "glob expression '${GLOBEXP}' does not refer to any files.")
else()
  file(MD5 ${TESTFILE} TESTHASH)
  set(output ${output} "Testfile:;${TESTHASH} - ${TESTFILE};Baselines:")
  foreach(FILEN ${REFFILES})
    file(MD5 ${FILEN} HASHN)
    set(output ${output} "${HASHN} - ${FILEN}")
    if(${TESTHASH} STREQUAL ${HASHN})
      set(HASH_FOUND TRUE)
    endif()
  endforeach()
endif()

if(NOT HASH_FOUND)
  set(output ${output} "Matching hash not found!")
  dumpoutput("${output}")
else()
  if(${CLEANTESTFILE})
    file(REMOVE ${TESTFILE})
  endif()
endif()
