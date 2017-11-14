# Generate data for folding Unicode strings
set(CASE_FOLD_DATA_FILE "${binary_dir}/vtkUnicodeCaseFoldData.h")
file(WRITE ${CASE_FOLD_DATA_FILE} "// Generated file, do not edit by hand!\n")
file(APPEND ${CASE_FOLD_DATA_FILE} "\n")
file(APPEND ${CASE_FOLD_DATA_FILE} "static vtkUnicodeString::value_type vtkUnicodeCaseFoldData[] = {\n")

# The following line relies on CMake 2.6, so for now we'll do it the old way
#file(STRINGS "${source_dir}/CaseFolding.txt" FOLDING)
file(READ "${source_dir}/CaseFolding.txt" FOLDING)
string(REGEX REPLACE ";" "\\\\;" FOLDING "${FOLDING}")
string(REGEX REPLACE "\n" ";" FOLDING "${FOLDING}")

foreach(FOLD ${FOLDING})
  if(FOLD MATCHES "^([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F]?); (C|F); ([^;]*); # (.*)")
    # The following 3 lines work in CMake 2.6, but not in 2.4,
    # so again we do it the old way.
    #SET(CODE ${CMAKE_MATCH_1})
    #SET(MAPPING ${CMAKE_MATCH_3})
    #SET(COMMENT ${CMAKE_MATCH_4})
    string(REGEX REPLACE  "^([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F]?); (C|F); ([^;]*); # (.*)"
      "\\1" CODE "${FOLD}")
    string(REGEX REPLACE  "^([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F]?); (C|F); ([^;]*); # (.*)"
      "\\3" MAPPING "${FOLD}")
    string(REGEX REPLACE  "^([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F]?); (C|F); ([^;]*); # (.*)"
      "\\4" COMMENT "${FOLD}")

    separate_arguments(MAPPING)

    file(APPEND ${CASE_FOLD_DATA_FILE} "  0x${CODE}, ")
    foreach(MAP ${MAPPING})
      file(APPEND ${CASE_FOLD_DATA_FILE} "0x${MAP}, ")
    endforeach()
    file(APPEND ${CASE_FOLD_DATA_FILE} "0x0000, // ${COMMENT}\n")
  endif()
endforeach()

file(APPEND ${CASE_FOLD_DATA_FILE} "  0x0000 };\n\n")
