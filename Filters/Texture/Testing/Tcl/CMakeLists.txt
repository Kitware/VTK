if(VTK_DATA_ROOT)
  vtk_add_test_tcl(AutomaticPlaneGeneration.tcl Graphics)
  vtk_add_test_tcl(cylMap.tcl Graphics)
  vtk_add_test_tcl(socbal.tcl Graphics)
  vtk_add_test_tcl(textureThreshold.tcl Graphics)
endif()

vtk_add_test_tcl(triangularTCoords.tcl Graphics)
