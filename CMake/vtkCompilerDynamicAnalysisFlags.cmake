# Determine if this is a sanitizer build
string (FIND "${CTEST_MEMORYCHECK_TYPE}" "Sanitizer" SANITIZER_BUILD)
if (${SANITIZER_BUILD} GREATER -1)
  # This is a sanitizer build.
  # Configure the sanitizer blacklist file
  set (SANITIZER_BLACKLIST "${VTK_BINARY_DIR}/sanitizer_blacklist.txt")
  configure_file (
    "${VTK_SOURCE_DIR}/Utilities/DynamicAnalysis/sanitizer_blacklist.txt.in"
    ${SANITIZER_BLACKLIST}
    @ONLY
    )

  # Add the compiler flags for blacklist
  set (FSANITIZE_BLACKLIST "\"-fsanitize-blacklist=${SANITIZER_BLACKLIST}\"")
  foreach (entity C CXX SHARED_LINKER EXE_LINKER MODULE_LINKER)
    set (CMAKE_${entity}_FLAGS "${CMAKE_${entity}_FLAGS} ${FSANITIZE_BLACKLIST}")
  endforeach ()
endif ()
