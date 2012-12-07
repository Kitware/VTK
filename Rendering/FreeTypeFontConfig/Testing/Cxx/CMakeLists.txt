# add tests that do not require data
set(MyTests
  TestSystemFontRendering
  )

# Use the testing object factory, to reduce boilerplate code in tests.
include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME Charts-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}.png
        -E ${_error_threshold})
  else()
    add_test(NAME Charts-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
