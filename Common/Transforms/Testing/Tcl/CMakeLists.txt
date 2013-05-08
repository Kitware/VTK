if(VTK_DATA_ROOT)
  vtk_add_test_tcl(cylindrical Graphics)
  vtk_add_test_tcl(spherical Graphics)
  vtk_add_test_tcl(TestThinPlateWarp Hybrid)
  vtk_add_test_tcl(TestThinPlateWarp3D Hybrid)
endif()

vtk_add_test_tcl(MatrixToTransform Graphics)
