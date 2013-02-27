set(MyTests "")

if(NOT APPLE)
  # This is from the old build system - not working on Apple platform.
  list(APPEND MyTests
    TestStructuredGridLIC2DXSlice.cxx
    TestStructuredGridLIC2DYSlice.cxx
    TestStructuredGridLIC2DZSlice.cxx
    )
  if(VTK_DATA_ROOT)
    list(APPEND MyTests
      TestSurfaceLIC.cxx
      TestImageDataLIC2D.cxx # This was segfaulting before modularization (and is now).
      )
  endif()
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    set(_basedir Hybrid)
    if("${TName}" STREQUAL "TestSurfaceLIC")
      set(_basedir Rendering)
    endif()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/${_basedir}/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
