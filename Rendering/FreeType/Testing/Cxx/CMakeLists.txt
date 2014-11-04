# Check that matplotlib is available
include(FindPythonModules)
find_python_module(matplotlib MATPLOTLIB_FOUND)

if(NOT MATPLOTLIB_FOUND)
  message(STATUS "Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests.")
endif()

# add tests that do not require data or produce vector output
set(TestFreeTypeTextMapperNoMath_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
set(TestMathTextFreeTypeTextRendererNoMath_ARGS
  DATA{../Data/Fonts/DejaVuSans.ttf}
)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestFTStringToPath.cxx
  TestFreeTypeTextMapperNoMath.cxx
  TestMathTextFreeTypeTextRendererNoMath.cxx
  TestTextActor.cxx
  TestTextActor3D.cxx
  TestTextActorAlphaBlending.cxx
  TestTextActorDepthPeeling.cxx
  TestTextActor3DAlphaBlending.cxx
  TestTextActor3DDepthPeeling.cxx
  )

if(MATPLOTLIB_FOUND)
  set(TestMathTextFreeTypeTextRenderer_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
  set(TestFreeTypeTextMapper_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})

  vtk_add_test_cxx(${vtk-module}CxxTests matplotlib_tests
    TestMathTextFreeTypeTextRenderer.cxx
    TestFreeTypeTextMapper.cxx
    )
  list(APPEND tests
    ${matplotlib_tests})
endif()

# This benchmark takes a while to run and can't fail, so disable it by default:
option(VTK_BUILD_FREETYPE_BENCHMARK
  "Build a BenchmarkFreeTypeRendering test that benchmarks freetype rendering performance."
  OFF)
mark_as_advanced(VTK_BUILD_FREETYPE_BENCHMARK)

if(VTK_BUILD_FREETYPE_BENCHMARK)
  vtk_add_test_cxx(${vtk-module}CxxTests bench_tests
    NO_VALID
    BenchmarkFreeTypeRendering.cxx
    )
  list(APPEND tests
    ${bench_tests})
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
