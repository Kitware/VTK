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
vtk_add_test_cxx(TestMathTextFreeTypeTextRendererNoMath.cxx)

if(VTK_DATA_ROOT)
  # add tests that require data
  vtk_add_test_cxx(TestFTStringToPath.cxx)
endif()

if(MATPLOTLIB_FOUND)
  vtk_add_test_cxx(TestMathTextFreeTypeTextRenderer.cxx)
endif()

# This benchmark takes a while to run and can't fail, so disable it by default:
option(VTK_BUILD_FREETYPE_BENCHMARK
  "Build a BenchmarkFreeTypeRendering test that benchmarks freetype rendering performance."
  OFF)
mark_as_advanced(VTK_BUILD_FREETYPE_BENCHMARK)

if(VTK_BUILD_FREETYPE_BENCHMARK)
  vtk_add_test_cxx(BenchmarkFreeTypeRendering.cxx NO_VALID)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
