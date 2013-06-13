# Tests with data
if(VTK_DATA_ROOT)
  set(NEEDS_DATA
  TestProStarReader.cxx
  TestTecplotReader.cxx
  TestAMRReadWrite.cxx
  )
endif()

set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()
if(VTK_LARGE_DATA_ROOT AND _known_little_endian)
  set( WINDBLADE_TEST
  TestWindBladeReader.cxx
  )
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${NEEDS_DATA}
  ${WINDBLADE_TEST}
  TestSimplePointsReaderWriter.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx ${WINDBLADE_TEST})

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/IO/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

if (WINDBLADE_TEST)
  add_test(NAME ${vtk-module}Cxx-TestWindBladeReader
    COMMAND ${vtk-module}CxxTests TestWindBladeReader
    -D ${VTK_LARGE_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/TestWindBladeReader.png)
endif()
