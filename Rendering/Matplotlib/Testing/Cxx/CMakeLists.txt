include(vtkGhostscript)

# Check that matplotlib is available
include(FindPythonModules)
find_python_module(matplotlib MATPLOTLIB_FOUND)

if(NOT MATPLOTLIB_FOUND)
  message(STATUS "Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests.")
endif()

if(MATPLOTLIB_FOUND)
  # add tests that require data
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestContextMathTextImage.cxx
    TestMathTextActor.cxx
    TestMathTextActor3D.cxx
    TestRenderString.cxx
    TestStringToPath.cxx
    TestIndexedLookupScalarBar.cxx
    TestScalarBarCombinatorics.cxx
    )

  if(VTK_GHOSTSCRIPT_EXECUTABLE)
    set(TestGL2PSFontDPIScaling_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
    vtk_add_test_cxx(${vtk-module}CxxTests GL2PSTests
      TestGL2PSFontDPIScaling.cxx
      TestGL2PSMathTextActor.cxx
      TestGL2PSMathTextActor3D.cxx
      TestGL2PSMathTextOutput.cxx
      TestGL2PSMathTextScaling.cxx
    )
    list(APPEND tests
      ${GL2PSTests})
  endif()

  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY)
endif()

include(vtkTestingGL2PS)
vtk_add_gl2ps_test_cxx(${GL2PSTests})
