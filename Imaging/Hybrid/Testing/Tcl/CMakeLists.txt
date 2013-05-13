# Tests with test images in Baseline/Graphics
#
if(VTK_DATA_ROOT)
  vtk_add_test_tcl(genHead Graphics)
endif()
vtk_add_test_tcl(iceCream Graphics)
vtk_add_test_tcl(shepards Graphics)
vtk_add_test_tcl(triangularTexture Graphics)
