# Check that matplotlib is available:
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import matplotlib"
  RESULT_VARIABLE IMPORT_MATPLOTLIB_EXITCODE
  OUTPUT_VARIABLE IMPORT_MATPLOTLIB_OUTPUT
  ERROR_VARIABLE IMPORT_MATPLOTLIB_ERROR
  )
set(MATPLOTLIB_FOUND FALSE)
if(${IMPORT_MATPLOTLIB_EXITCODE} EQUAL 0)
  set(MATPLOTLIB_FOUND TRUE)
endif()

if(NOT MATPLOTLIB_FOUND)
  message(STATUS "Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests.")
endif()

# add tests that do not require data or produce vector output
set(MyTests
  TestMathTextFreeTypeTextRendererNoMath.cxx
  )

if(VTK_DATA_ROOT)
  # add tests that require data
  set(MyTests ${MyTests}
    TestFTStringToPath.cxx
    )
endif()

if(MATPLOTLIB_FOUND)
  set(MyTests ${MyTests}
    TestMathTextFreeTypeTextRenderer.cxx
    )
endif()

# This benchmark takes a while to run and can't fail, so disable it by default:
option(VTK_BUILD_FREETYPE_BENCHMARK
  "Build a BenchmarkFreeTypeRendering test that benchmarks freetype rendering performance."
  OFF)
mark_as_advanced(VTK_BUILD_FREETYPE_BENCHMARK)

if(VTK_BUILD_FREETYPE_BENCHMARK)
  set(MyTests ${MyTests} BenchmarkFreeTypeRendering.cxx)
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
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}.png
        -E ${_error_threshold})
  else()
    add_test(NAME FreeType-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
