# Compile the PNGCompare test, which is used to validate PNG images (read from
# disk, not generated in VTK). The CMake/RasterizePostScript script can be used
# to create a png image from GL2PS output.
set_property(DIRECTORY APPEND PROPERTY VTK_TEST_CXX_SOURCES PNGCompare.cxx)
vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
