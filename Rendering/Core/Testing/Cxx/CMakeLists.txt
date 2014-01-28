vtk_add_test_cxx(
  FrustumClip.cxx
  Mace.cxx
  RGrid.cxx
  TestAssemblyBounds.cxx,NO_VALID
  TestPickingManager.cxx,NO_VALID
  TestActor2DTextures.cxx
  TestBareScalarsToColors.cxx
  TestMapVectorsAsRGBColors.cxx
  TestMapVectorsToColors.cxx
  TestSplitViewportStereoHorizontal.cxx
  otherLookupTable.cxx
  otherLookupTableWithEnabling.cxx
  # TestViewDependentErrorMetric.cxx # Loads of extra deps - move?
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
