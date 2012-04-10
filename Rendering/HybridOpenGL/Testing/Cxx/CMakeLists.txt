set(MyTests
  TestSurfaceLIC.cxx
  )

if(NOT APPLE)
  # This is from the old build system - not working on Apple platform.
  list(APPEND MyTests
    TestImageDataLIC2D.cxx # This was segfaulting before modularization (and is now).
    TestStructuredGridLIC2DXSlice.cxx
    TestStructuredGridLIC2DYSlice.cxx
    TestStructuredGridLIC2DZSlice.cxx
    )
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TestSurfaceLIC.cxx)
add_test(NAME ${vtk-module}Cxx-SurfaceLIC
  COMMAND ${vtk-module}CxxTests TestSurfaceLIC
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Rendering/TestSurfaceLIC.png)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Hybrid/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
