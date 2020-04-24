vtk_add_test_python(
  Disk.py
  OSCone.py
  TestButtonSource.py
  TestStaticCellLocatorLineIntersection.py
  TestStaticPointLocatorLineIntersection.py
  TestRegularPolygonSource.py
  coneResolution.py
  glyph2D.py
  TestPlatonicSolids.py,NO_RT
  TestGlyphScaling.py
  TestEllipseArcSourceResolution.py
  )
if(TARGET VTK::RenderingTk)
  vtk_add_test_python(
    squadViewer.py,NO_RT
    )
endif()
