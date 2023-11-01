set(extra_opengl2_tests)
if (VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  list(APPEND extra_opengl2_tests TestOffscreenIsOffscreen.cxx,NO_DATA,NO_VALID)
endif()
if (VTK_USE_X)
  list(APPEND extra_opengl2_tests TestInteractorTimers.cxx,NO_VALID)
endif()

vtk_add_test_cxx(vtkRenderingCoreCxxTests tests
  ${extra_opengl2_tests}
  FrustumClip.cxx
  Mace.cxx
  otherCoordinate.cxx,NO_DATA,NO_VALID
  otherLookupTable.cxx,NO_VALID
  otherLookupTableWithEnabling.cxx,NO_VALID
  RenderNonFinite.cxx
  RGrid.cxx
  SurfacePlusEdges.cxx
  TestActor2D.cxx
  TestActor2DTextures.cxx
  TestActorLightingFlag.cxx
  TestAnimationScene.cxx
  TestAreaSelections.cxx
  TestAssemblyBounds.cxx,NO_VALID
  TestBackfaceCulling.cxx
  TestBareScalarsToColors.cxx
  TestBackfaceTexture.cxx
  TestBlockOpacity.cxx
  TestBlockVisibility.cxx
  TestColorByCellDataStringArray.cxx
  TestColorByPointDataStringArray.cxx
  TestColorByStringArrayDefaultLookupTable.cxx
  TestColorByStringArrayDefaultLookupTable2D.cxx
  TestColorTransferFunction.cxx,NO_VALID
  TestColorTransferFunctionStringArray.cxx,NO_VALID
  TestCompositeDataDisplayAttributes.cxx,NO_VALID
  TestCompositePolyDataMapper.cxx,NO_DATA
  TestCompositePolyDataMapperCameraShiftScale.cxx,NO_DATA
  TestCompositePolyDataMapperCellScalars.cxx,NO_DATA
  TestCompositePolyDataMapperCustomShader.cxx,NO_DATA
  TestCompositePolyDataMapperMixedGeometryCellScalars.cxx,NO_DATA
  TestCompositePolyDataMapperMixedGeometryEdges.cxx,NO_DATA
  TestCompositePolyDataMapperNaNPartial.cxx,NO_DATA
  TestCompositePolyDataMapperOverrideScalarArray.cxx,NO_DATA
  TestCompositePolyDataMapperOverrideLUT.cxx,NO_DATA
  TestCompositePolyDataMapperPickability.cxx,NO_DATA
  TestCompositePolyDataMapperPicking.cxx,NO_DATA
  TestCompositePolyDataMapperScalars.cxx,NO_DATA
  TestCompositePolyDataMapperScalarsSurfaceOpacity.cxx,NO_DATA
  TestCompositePolyDataMapperSharedArray.cxx,NO_DATA
  TestCompositePolyDataMapperStaticBounds.cxx,NO_DATA,NO_VALID
  TestCompositePolyDataMapperSpheres.cxx,NO_DATA
  TestCompositePolyDataMapperVertices.cxx,NO_DATA
  TestDirectScalarsToColors.cxx
  TestDiscretizableColorTransferFunction.cxx,NO_VALID
  TestDiscretizableColorTransferFunctionOpacity.cxx,NO_VALID
  TestDiscretizableColorTransferFunctionStringArray.cxx,NO_VALID
  TestDragEvent.cxx
  TestEdgeFlags.cxx
  TestEdgeOpacity.cxx
  TestFollowerPicking.cxx
  TestGlyph3DMapper.cxx
  TestGlyph3DMapper2.cxx,NO_VALID,NO_RT,NO_DATA
  TestGlyph3DMapperArrow.cxx
  TestGlyph3DMapperIndexing.cxx,NO_DATA
  TestGlyph3DMapperMasking.cxx
  TestGlyph3DMapperOrientationArray.cxx
  TestGlyph3DMapperPicking.cxx
  TestGlyph3DMapperQuaternionArray.cxx
  TestGlyph3DMapperTreeIndexing.cxx,NO_DATA
  TestGradientBackground.cxx
  TestHomogeneousTransformOfActor.cxx
  TestImageAndAnnotations.cxx,NO_DATA
  TestInteractorStyleImageProperty.cxx,NO_VALID
  TestLabeledContourMapper.cxx
  TestLabeledContourMapperNoLabels.cxx
  TestLabeledContourMapperWithActorMatrix.cxx
  TestManyActors.cxx,NO_VALID
  TestMapVectorsAsRGBColors.cxx
  TestMapVectorsToColors.cxx
  TestMultiBlockPartialArrayFieldData.cxx
  TestMultiBlockPartialArrayPointData.cxx
  TestOffAxisStereo.cxx
  TestOnAndOffScreenConeCxx.cxx
  TestOpacity.cxx
  TestOrderedTriangulator.cxx
  TestOSConeCxx.cxx
  TestPickingManager.cxx,NO_VALID
  TestPickTextActor.cxx,NO_VALID
  TestPointSelection.cxx,NO_VALID
  TestPointSelectionWithCellData.cxx,NO_VALID
  TestPolygonSelection.cxx
  TestResetCameraScreenSpace.cxx
  TestResetCameraVerticalAspectRatio.cxx
  TestResetCameraVerticalAspectRatioParallel.cxx
  TestSelectVisiblePoints.cxx,NO_VALID
  TestSplitViewportStereoHorizontal.cxx
  TestStereoBackgroundLeft.cxx
  TestStereoBackgroundRight.cxx
  TestTexturedBackground.cxx
  TestTexturedCylinder.cxx
  TestTextureRGBA.cxx
  TestTextureRGBADepthPeeling.cxx
  TestTextureSize.cxx,NO_VALID
  TestTilingCxx.cxx
  TestToggleOSWithInteractor.cxx,NO_DATA
  TestTransformCoordinateUseDouble.cxx
  TestTranslucentImageActorAlphaBlending.cxx
  TestTranslucentImageActorDepthPeeling.cxx
  TestTranslucentLUTAlphaBlending.cxx
  TestTranslucentLUTDepthPeeling.cxx
  TestTranslucentLUTTextureAlphaBlending.cxx
  TestTranslucentLUTTextureDepthPeeling.cxx
  TestTStripsColorsTCoords.cxx
  TestTStripsNormalsColorsTCoords.cxx
  TestTStripsNormalsTCoords.cxx
  TestTStripsTCoords.cxx
  TestTupleInterpolator.cxx,NO_VALID,NO_DATA
  TestWindowToImageFilter.cxx
  TestResizingWindowToImageFilter.cxx
  TestTextureWrap.cxx
  # TestViewDependentErrorMetric.cxx # Loads of extra deps - move?
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )

set_source_files_properties(
  TestEdgeFlags.cxx # fails on OpenGL Mesa
  PROPERTIES COMPILE_FLAGS -DVTK_OPENGL2)

if (VTK_USE_X)
  set_tests_properties(VTK::RenderingCoreCxx-TestInteractorTimers
     PROPERTIES RUN_SERIAL ON)
endif ()

vtk_test_cxx_executable(vtkRenderingCoreCxxTests tests)
