if(NOT APPLE)
  # This is from the old build system - not working on Apple platform.
  vtk_add_test_cxx(
    TestStructuredGridLIC2DXSlice.cxx
    TestStructuredGridLIC2DYSlice.cxx
    TestStructuredGridLIC2DZSlice.cxx
    )
  vtk_add_test_cxx(TestSurfaceLIC.cxx)
  vtk_add_test_cxx(TestImageDataLIC2D.cxx) # This was segfaulting before modularization (and is now).
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
