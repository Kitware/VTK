vtk_add_test_python(
  coneResolution.py
  glyph2D.py
  Disk.py
  OSCone.py
  TestButtonSource.py
# TestEllipseArcSourceResolution.py #19221
  TestExponentialPointSource.py
  TestGlyphScaling.py
  TestPlatonicNormals.py
  TestPlatonicSolids.py
  TestRegularPolygonSource.py
  TestStaticCellLocatorLineIntersection.py
  TestStaticPointLocatorLineIntersection.py
  )
if(TARGET VTK::RenderingTk)
  vtk_add_test_python(
    squadViewer.py
    )
endif()
