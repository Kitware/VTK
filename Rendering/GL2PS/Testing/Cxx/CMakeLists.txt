# Compile the PNGCompare test, which is used to validate PNG images (read from
# disk, not generated in VTK). The CMake/RasterizePostScript script can be used
# to create a png image from GL2PS output.
set(MyTests PNGCompare.cxx)

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})
