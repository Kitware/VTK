vtk_add_test_cxx(${vtk-module}CxxTests tests
  FrustumClip.cxx
  Mace.cxx
  RGrid.cxx
  TestActor2DTextures.cxx
  TestActorLightingFlag.cxx
  TestAnimationScene.cxx
  TestAreaSelections.cxx
  TestAssemblyBounds.cxx,NO_VALID
  TestBackfaceCulling.cxx
  TestBareScalarsToColors.cxx
  TestFollowerPicking.cxx
  TestGlyph3DMapper.cxx
  TestGlyph3DMapperMasking.cxx
  TestGlyph3DMapperOrientationArray.cxx
  TestGlyph3DMapperPicking.cxx
  TestPickingManager.cxx,NO_VALID
  TestMapVectorsAsRGBColors.cxx
  TestMapVectorsToColors.cxx
  TestSplitViewportStereoHorizontal.cxx
  otherLookupTable.cxx,NO_VALID
  otherLookupTableWithEnabling.cxx,NO_VALID
  RenderNonFinite.cxx
  SurfacePlusEdges.cxx
  # TestViewDependentErrorMetric.cxx # Loads of extra deps - move?
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
