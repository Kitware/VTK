# Tests with test images in Baseline/Graphics
#
if(VTK_DATA_ROOT)
  vtk_add_test_tcl(genHead)
endif()
vtk_add_test_tcl(iceCream)
vtk_add_test_tcl(shepards)
vtk_add_test_tcl(triangularTexture)
