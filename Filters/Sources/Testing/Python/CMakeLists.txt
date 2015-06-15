vtk_add_test_python(
  Disk.py
  OSCone.py
  TestButtonSource.py
  TestRegularPolygonSource.py
  coneResolution.py
  glyph2D.py
  TestPlatonicSolids.py,NO_RT
  )
if(VTK_USE_TK AND Module_vtkRenderingTk)
  vtk_add_test_python(
    squadViewer.py,NO_RT
    )
endif()
