include(../../../../CMake/vtkGhostscript.cmake)

# Check that matplotlib is available
include(../../../../CMake/FindPythonModules.cmake)
find_python_module(matplotlib MATPLOTLIB_FOUND)

if(MATPLOTLIB_FOUND)
  # add tests that require data
  vtk_add_test_cxx(vtkRenderingMatplotlibCxxTests tests
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
    vtk_add_test_cxx(vtkRenderingMatplotlibCxxTests GL2PSTests
      TestGL2PSFontDPIScaling.cxx
      TestGL2PSMathTextActor.cxx
      TestGL2PSMathTextActor3D.cxx
      TestGL2PSMathTextOutput.cxx
      TestGL2PSMathTextScaling.cxx
    )
    list(APPEND tests
      ${GL2PSTests})
  endif()

  vtk_test_cxx_executable(vtkRenderingMatplotlibCxxTests tests
    RENDERING_FACTORY)
endif()

include(../../../../CMake/vtkTestingGL2PS.cmake)
vtk_add_gl2ps_test_cxx(${GL2PSTests})
