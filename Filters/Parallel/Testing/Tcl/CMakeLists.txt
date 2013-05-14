if(VTK_DATA_ROOT)
  vtk_add_test_tcl(RectOutline.tcl)
  vtk_add_test_tcl(TestExtractCTHPart.tcl)
  vtk_add_test_tcl(TestImageStreamer.tcl)
endif()

vtk_add_test_tcl(TestCutMaterial.tcl)
vtk_add_test_tcl(TestExtrudePiece.tcl)
