if(VTK_DATA_ROOT)
  add_test_tcl(cylindrical Graphics)
  add_test_tcl(spherical Graphics)
  add_test_tcl(TestThinPlateWarp Hybrid)
  add_test_tcl(TestThinPlateWarp3D Hybrid)
endif()

add_test_tcl(MatrixToTransform Graphics)
