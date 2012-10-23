set(TEST_SRC)

if(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND TEST_SRC TestAVIWriter.cxx)
endif()

if(vtkIOMovie_vtkoggtheora)
  list(APPEND TEST_SRC TestOggTheoraWriter.cxx)
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  # TestMovieWriter.cxx           # fixme (deps not satisfied)
  ${TEST_SRC}
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

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
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
