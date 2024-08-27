# Declare unit tests Usage:
#
# token_unit_tests(
#   LABEL <prefix for all unit tests>
#   SOURCES <test_source_list>
#   SOURCES_REQUIRE_DATA <test_sources_that_require_token_DATA_DIR>
#   EXTRA_SOURCES <helper_source_files>
#   LIBRARIES <dependent_library_list>
#   )
function(token_unit_tests)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs LABEL SOURCES SOURCES_SERIAL SOURCES_REQUIRE_DATA SOURCES_SERIAL_REQUIRE_DATA EXTRA_SOURCES LIBRARIES)
  cmake_parse_arguments(token_ut
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  set(have_testing_data OFF)
  if (token_DATA_DIR)
    set(have_testing_data ON)
    list(APPEND token_ut_SOURCES ${token_ut_SOURCES_REQUIRE_DATA})
    list(APPEND token_ut_SOURCES_SERIAL ${token_ut_SOURCES_SERIAL_REQUIRE_DATA})
  endif()
  list(APPEND token_ut_SOURCES ${token_ut_SOURCES_SERIAL})

  list(LENGTH token_ut_SOURCES num_sources)
  if(NOT ${num_sources})
    #no sources don't make a target
    return()
  endif()

  if (token_ENABLE_TESTING)
    set(kit "token") # One day we may need: token_get_kit_name(kit)
    #we use UnitTests_ so that it is an unique key to exclude from coverage
    set(test_prog UnitTests_${kit})

    create_test_sourcelist(TestSources ${test_prog}.cxx ${token_ut_SOURCES})
    add_executable(${test_prog} ${TestSources} ${token_ut_EXTRA_SOURCES})

    target_link_libraries(${test_prog} LINK_PRIVATE ${token_ut_LIBRARIES})
    target_include_directories(${test_prog}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${MOAB_INCLUDE_DIRS}
        ${VTK_INCLUDE_DIRS}
        )

    target_compile_definitions(${test_prog} PRIVATE "token_SCRATCH_DIR=\"${CMAKE_BINARY_DIR}/Testing/Temporary\"")
    if(have_testing_data)
      target_compile_definitions(${test_prog} PRIVATE "token_DATA_DIR=\"${token_DATA_DIR}\"")
    endif()

    foreach (test ${token_ut_SOURCES})
      get_filename_component(tname ${test} NAME_WE)
      add_test(NAME ${tname}
        COMMAND ${test_prog} ${tname} ${${tname}_EXTRA_ARGUMENTS}
        )
      set_tests_properties(${tname} PROPERTIES TIMEOUT 120)
      if(token_ut_LABEL)
        set_tests_properties(${tname} PROPERTIES LABELS "${token_ut_LABEL}")
      endif()
    endforeach()

    foreach (test ${token_ut_SOURCES_SERIAL})
      get_filename_component(tname ${test} NAME_WE)
      set_tests_properties(${tname} PROPERTIES RUN_SERIAL TRUE)
    endforeach()

  endif ()
endfunction()
