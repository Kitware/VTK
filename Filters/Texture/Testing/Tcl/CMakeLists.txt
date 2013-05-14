if(VTK_DATA_ROOT)
  vtk_add_test_tcl(AutomaticPlaneGeneration.tcl)
  vtk_add_test_tcl(cylMap.tcl)
  vtk_add_test_tcl(socbal.tcl)
  vtk_add_test_tcl(textureThreshold.tcl)
endif()

vtk_add_test_tcl(triangularTCoords.tcl)
