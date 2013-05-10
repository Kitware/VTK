if(NOT APPLE)
  # This is from the old build system - not working on Apple platform.
  vtk_add_test_cxx(Hybrid
    TestStructuredGridLIC2DXSlice.cxx
    TestStructuredGridLIC2DYSlice.cxx
    TestStructuredGridLIC2DZSlice.cxx
    )
  if(VTK_DATA_ROOT)
    vtk_add_test_cxx(TestSurfaceLIC.cxx Rendering)
    vtk_add_test_cxx(TestImageDataLIC2D.cxx Hybrid) # This was segfaulting before modularization (and is now).
  endif()
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
