# Tests with test images in Baseline/Graphics
#
if(VTK_DATA_ROOT)
  add_test_tcl(genHead Graphics)
endif()
add_test_tcl(iceCream Graphics)
add_test_tcl(shepards Graphics)
add_test_tcl(triangularTexture Graphics)
