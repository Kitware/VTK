## Compile the PNGCompare test, which is used to validate PNG images (read from
## disk, not generated in VTK). The CMake/RasterizePostScript script can be used
## to create a png image from GL2PS output.
#set(tests PNGCompare TestGL2PSAddPolyPrimitive)
#vtk_test_cxx_executable(${vtk-module}CxxTests tests
#  RENDERING_FACTORY
#  )




set(GL2PSTests
  TestGL2PSAddPolyPrimitive.cxx
)

vtk_add_test_cxx(${vtk-module}CxxTests tests NO_VALID ${GL2PSTests})

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
