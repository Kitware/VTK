set(TEST_SRC)

if(WIN32)
  list(APPEND TEST_SRC TestAVIWriter.cxx)
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  # TestFFMPEGWriter.cxx          # fixme (deps not satisfied)
  # TestImageReader2Factory.cxx   # fixme (deps not satisfied)
  TestMetaIO.cxx
  # TestMovieWriter.cxx           # fixme (deps not satisfied)
  # TestOggTheoraWriter.cxx       # fixme (deps not satisfied)
  ${TEST_SRC}
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TestMetaIO.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/IO/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-MetaIO
    COMMAND ${vtk-module}CxxTests TestMetaIO
      ${VTK_DATA_ROOT}/Data/HeadMRVolume.mhd)
endif()
