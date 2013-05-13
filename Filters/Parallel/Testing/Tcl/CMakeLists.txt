if(VTK_DATA_ROOT)
  vtk_add_test_tcl(RectOutline.tcl Parallel)
  vtk_add_test_tcl(TestExtractCTHPart.tcl Parallel)
  vtk_add_test_tcl(TestImageStreamer.tcl Parallel)
endif()

vtk_add_test_tcl(TestCutMaterial.tcl Parallel)
vtk_add_test_tcl(TestExtrudePiece.tcl Parallel)
