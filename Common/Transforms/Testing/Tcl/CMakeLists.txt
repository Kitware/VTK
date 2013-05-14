if(VTK_DATA_ROOT)
  vtk_add_test_tcl(cylindrical)
  vtk_add_test_tcl(spherical)
  vtk_add_test_tcl(TestThinPlateWarp)
  vtk_add_test_tcl(TestThinPlateWarp3D)
endif()

vtk_add_test_tcl(MatrixToTransform)
