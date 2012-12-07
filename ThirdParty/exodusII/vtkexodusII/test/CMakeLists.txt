set(cbind_TEST_SRCS
  CreateEdgeFace.c
  ReadEdgeFace.c
)

set_source_files_properties(
  CreateEdgeFace.c
  ReadEdgeFace.c
  COMPILE_FLAGS -DEXO_CTEST
)

# Always report error/warning messages during testing:
# The ex_opts call precipitates the EXTRA_INCLUDE below.
set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
  "${CMAKE_TESTDRIVER_BEFORE_TESTMAIN}  ex_opts( EX_VERBOSE );\n"
)
create_test_sourcelist(ExoIICTests
  ExoIICTests.cxx
  ${cbind_TEST_SRCS}
  EXTRA_INCLUDE exodusII_test.h
  )

add_executable(ExoIICTests ${ExoIICTests})
target_link_libraries(ExoIICTests vtkCommon vtkexoIIc vtkNetCDF)

set(TestsToRun ${ExoIICTests})
list(REMOVE_ITEM TestsToRun ExoIICTests.cxx)

foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  add_test(NAME ${TName} COMMAND ExoIICTests ${TName})
endforeach()
add_test(NAME CreateEdgeFaceWithConcats
  COMMAND ExoIICTests CreateEdgeFace -pcab -pcset -pvpax )
add_test(NAME ReadEdgeFaceWithConcats
  COMMAND ExoIICTests ReadEdgeFace )

