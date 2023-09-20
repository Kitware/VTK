function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

file(WRITE "${ALL_SQL_IN}" "")
include(sql_filelist.cmake)
foreach(SQL_FILE ${SQL_FILES})
  cat(${SQL_FILE} "${ALL_SQL_IN}")
endforeach()

# Do ${PROJ_VERSION} substitution
file(READ ${ALL_SQL_IN} CONTENTS)
string(REPLACE "\${PROJ_VERSION}" "${PROJ_VERSION}" CONTENTS_MOD "${CONTENTS}")
file(WRITE "${ALL_SQL_IN}" "${CONTENTS_MOD}")

# XXX(kitware): For emscripten, input pipe appears to have some issue which hangs the command.
#               Instead of piping ALL_SQL_IN, use the '-init' option for sqlitebin as a workaround
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  execute_process(COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${EXE_SQLITE3}" "-init" "${ALL_SQL_IN}" "${PROJ_DB}"
                  COMMAND_ECHO STDOUT
                  RESULT_VARIABLE STATUS)
else()
  execute_process(COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${EXE_SQLITE3}" "${PROJ_DB}"
                  INPUT_FILE "${ALL_SQL_IN}"
                  RESULT_VARIABLE STATUS)
endif()
if(STATUS AND NOT STATUS EQUAL 0)
  message(FATAL_ERROR "SQLite3 failed")
endif()
