if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(extra_opengl2_tests
    TestCompositePolyDataMapper2.cxx,NO_DATA
    TestCompositePolyDataMapper2Picking.cxx,NO_DATA
    TestCompositePolyDataMapper2Scalars.cxx,NO_DATA
    TestCompositePolyDataMapper2CellScalars.cxx,NO_DATA
    TestCompositePolyDataMapper2MixedGeometryCellScalars.cxx,NO_DATA
    TestCompositePolyDataMapper2MixedGeometryEdges.cxx,NO_DATA
    TestMultiBlockPartialArrayPointData.cxx
    TestMultiBlockPartialArrayFieldData.cxx
    # the following produce incorrect results in OpenGL
    TestTranslucentLUTAlphaBlending.cxx
    TestTranslucentLUTTextureAlphaBlending.cxx
    TestAreaSelections.cxx
    TestValuePassFloatingPoint.cxx
    TestToggleOSWithInteractor.cxx,NO_DATA
    )
else()
  set(extra_opengl2_tests
    TestCompositePolyDataMapper2.cxx,NO_DATA,NO_VALID
    TestCompositePolyDataMapper2Scalars.cxx,NO_DATA,NO_VALID
    TestCompositePolyDataMapper2CellScalars.cxx,NO_DATA,NO_VALID
    TestCompositePolyDataMapper2MixedGeometryCellScalars.cxx,NO_DATA,NO_VALID
    TestCompositePolyDataMapper2MixedGeometryEdges.cxx,NO_DATA,NO_VALID
    )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${extra_opengl2_tests}
  otherCoordinate.cxx,NO_DATA,NO_VALID
  FrustumClip.cxx
  Mace.cxx
  RGrid.cxx
  TestActor2D.cxx
  TestActor2DTextures.cxx
  TestActorLightingFlag.cxx
  TestAnimationScene.cxx
  TestAssemblyBounds.cxx,NO_VALID
  TestBackfaceCulling.cxx
  TestBareScalarsToColors.cxx
  TestBlockOpacity.cxx
  TestColorByCellDataStringArray.cxx
  TestColorByPointDataStringArray.cxx
  TestColorByStringArrayDefaultLookupTable.cxx
  TestColorByStringArrayDefaultLookupTable2D.cxx
  TestColorTransferFunction.cxx,NO_VALID
  TestColorTransferFunctionStringArray.cxx,NO_VALID
  TestDirectScalarsToColors.cxx
  TestDiscretizableColorTransferFunction.cxx,NO_VALID
  TestDiscretizableColorTransferFunctionStringArray.cxx,NO_VALID
  TestEdgeFlags.cxx
  TestFollowerPicking.cxx
  TestGlyph3DMapper.cxx
  TestGlyph3DMapper2.cxx,NO_VALID,NO_RT,NO_DATA
  TestGlyph3DMapperArrow.cxx
  TestGlyph3DMapperMasking.cxx
  TestGlyph3DMapperOrientationArray.cxx
  TestGlyph3DMapperPicking.cxx
  TestGradientBackground.cxx
  TestHiddenLineRemovalPass.cxx
  TestHomogeneousTransformOfActor.cxx
  TestImageAndAnnotations.cxx,NO_DATA
  TestInteractorStyleImageProperty.cxx,NO_VALID
  TestInteractorTimers.cxx,NO_VALID
  TestLabeledContourMapper.cxx
  TestLabeledContourMapperWithActorMatrix.cxx
  TestPickingManager.cxx,NO_VALID
  TestManyActors.cxx,NO_VALID
  TestMapVectorsAsRGBColors.cxx
  TestMapVectorsToColors.cxx
  TestOffAxisStereo.cxx
  TestOrderedTriangulator.cxx
  TestOpacity.cxx
  TestOSConeCxx.cxx
  TestPickTextActor.cxx,NO_VALID
  TestPointSelection.cxx,NO_VALID
  TestPolygonSelection.cxx
  TestResetCameraVerticalAspectRatio.cxx
  TestResetCameraVerticalAspectRatioParallel.cxx
  TestSplitViewportStereoHorizontal.cxx
  TestTexturedBackground.cxx
  TestTextureSize.cxx,NO_VALID
  TestTextureRGBA.cxx
  TestTextureRGBADepthPeeling.cxx
  TestTilingCxx.cxx
  TestTransformCoordinateUseDouble.cxx
  TestTranslucentImageActorAlphaBlending.cxx
  TestTranslucentImageActorDepthPeeling.cxx
  TestTranslucentLUTDepthPeeling.cxx
  TestTranslucentLUTTextureDepthPeeling.cxx
  TestTStripsColorsTCoords.cxx
  TestTStripsNormalsColorsTCoords.cxx
  TestTStripsNormalsTCoords.cxx
  TestTStripsTCoords.cxx
  TestWindowToImageFilter.cxx
  otherLookupTable.cxx,NO_VALID
  otherLookupTableWithEnabling.cxx,NO_VALID
  RenderNonFinite.cxx
  SurfacePlusEdges.cxx
  # TestViewDependentErrorMetric.cxx # Loads of extra deps - move?
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set_source_files_properties(
   TestEdgeFlags.cxx # fails on OpenGL Mesa
    PROPERTIES COMPILE_FLAGS -DVTK_OPENGL2)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
