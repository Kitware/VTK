include(vtkGhostscript)
include(vtkTestingGL2PS)

set(GL2PSTests)
if(VTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    TestGL2PSAddPolyPrimitive.cxx
  )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests NO_VALID ${GL2PSTests})

# Create rasterize/verify tests to check generated postscript:
vtk_add_gl2ps_test_cxx(${GL2PSTests})

# Compile the PNGCompare test, which is used to validate PNG images (read from
# disk, not generated in VTK).
list(APPEND tests PNGCompare)

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
