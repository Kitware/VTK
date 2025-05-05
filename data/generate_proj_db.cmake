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

execute_process(COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${EXE_SQLITE3}" "${PROJ_DB}"
                INPUT_FILE "${ALL_SQL_IN}"
                RESULT_VARIABLE STATUS)

if(STATUS AND NOT STATUS EQUAL 0)
  message(FATAL_ERROR "SQLite3 failed")
endif()
