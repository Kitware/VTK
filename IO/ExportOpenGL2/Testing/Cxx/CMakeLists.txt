include(vtkGhostscript)
include(vtkTestingGL2PS)

set(GL2PSTests)
if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    # These tests differ from OpenGL1, hence they're in this submodule.
    TestGL2PSContext.cxx # No image export supported in OGL
  )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests ${GL2PSTests})

# Create rasterize/verify tests to check generated postscript:
vtk_add_gl2ps_test_cxx(${GL2PSTests})

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
