if(VTK_DATA_ROOT)
  add_test_tcl(RectOutline.tcl Parallel)
  add_test_tcl(TestExtractCTHPart.tcl Parallel)
  add_test_tcl(TestImageStreamer.tcl Parallel)
endif()

add_test_tcl(TestCutMaterial.tcl Parallel)
add_test_tcl(TestExtrudePiece.tcl Parallel)
