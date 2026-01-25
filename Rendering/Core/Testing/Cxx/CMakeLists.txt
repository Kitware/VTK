set(extra_opengl2_tests)
if (VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  list(APPEND extra_opengl2_tests TestOffscreenIsOffscreen.cxx,NO_DATA,NO_VALID)
endif()
if (VTK_USE_X)
  list(APPEND extra_opengl2_tests TestInteractorTimers.cxx,NO_VALID)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()

set(cxx_tests
  FrustumClip.cxx
  Mace.cxx
  otherCoordinate.cxx,NO_DATA,NO_VALID
  otherLookupTable.cxx,NO_VALID
  otherLookupTableWithEnabling.cxx,NO_VALID
  RenderNonFinite.cxx
  RGrid.cxx
# SurfacePlusEdges.cxx #19221
  TestActor2D.cxx
  TestActor2DTextures.cxx
  TestActorFaceCullingProperty.cxx,NO_DATA
  TestActorLightingFlag.cxx
  TestAnimationScene.cxx
  TestAreaSelections.cxx
  TestAssemblyBounds.cxx,NO_VALID
  TestAxesActor.cxx,NO_DATA
  TestBackfaceCulling.cxx
  TestBareScalarsToColors.cxx
  TestBackfaceTexture.cxx
  TestBlockOpacity.cxx
  TestBlockVisibility.cxx
  TestCellScalarMappedColors.cxx,NO_DATA
  TestColorByCellDataStringArray.cxx
  TestColorByPointDataStringArray.cxx
  TestColorByStringArrayDefaultLookupTable.cxx
  TestColorByStringArrayDefaultLookupTable2D.cxx
  TestColorTransferFunction.cxx,NO_VALID
  TestColorTransferFunctionStringArray.cxx,NO_VALID
  TestCompositeDataDisplayAttributes.cxx,NO_VALID
  TestCompositePolyDataMapper.cxx,NO_DATA
  TestCompositePolyDataMapperBlockOpacities.cxx,NO_DATA
  TestCompositePolyDataMapperBlockTextures.cxx,NO_DATA
  TestCompositePolyDataMapperCameraShiftScale.cxx,NO_DATA
  TestCompositePolyDataMapperCellScalars.cxx,NO_DATA
  TestCompositePolyDataMapperCustomShader.cxx,NO_DATA
  TestCompositePolyDataMapperMixedGeometryCellScalars.cxx,NO_DATA
  TestCompositePolyDataMapperMixedGeometryEdges.cxx,NO_DATA
  TestCompositePolyDataMapperNaNPartial.cxx,NO_DATA
  TestCompositePolyDataMapperOverrideScalarArray.cxx,NO_DATA
  TestCompositePolyDataMapperOverrideLUT.cxx,NO_DATA
  TestCompositePolyDataMapperPartialFieldData.cxx,NO_DATA
  TestCompositePolyDataMapperPartialPointData.cxx,NO_DATA
  TestCompositePolyDataMapperPickability.cxx,NO_DATA
  TestCompositePolyDataMapperPicking.cxx,NO_DATA
  TestCompositePolyDataMapperScalars.cxx,NO_DATA
# TestCompositePolyDataMapperScalarsSurfaceOpacity.cxx,NO_DATA #19221
  TestCompositePolyDataMapperSharedArray.cxx,NO_DATA
  TestCompositePolyDataMapperStaticBounds.cxx,NO_DATA,NO_VALID
  TestCompositePolyDataMapperSpheres.cxx,NO_DATA
  TestCompositePolyDataMapperToggleScalarVisibilities.cxx,NO_DATA
  TestCompositePolyDataMapperVertices.cxx,NO_DATA
  TestDirectScalarsToColors.cxx
  TestDiscretizableColorTransferFunction.cxx,NO_VALID
  TestDiscretizableColorTransferFunctionOpacity.cxx,NO_VALID
  TestDiscretizableColorTransferFunctionStringArray.cxx,NO_VALID
  TestDragEvent.cxx,NO_SERDES
  TestEdgeFlags.cxx
  TestEdgeOpacity.cxx
  TestEdgeThickness.cxx
  TestFollowerPicking.cxx
  TestGlyph3DMapper.cxx
  TestGlyph3DMapperPointSize.cxx
  TestGlyph3DMapper2.cxx,NO_VALID,NO_RT,NO_DATA
  TestGlyph3DMapperArrow.cxx
  TestGlyph3DMapperBackfaceColor.cxx
  TestGlyph3DMapperBounds.cxx,NO_DATA,NO_VALID
  TestGlyph3DMapperCompositeDisplayAttributeInheritance.cxx,NO_DATA
  TestGlyph3DMapperIndexing.cxx,NO_DATA
  TestGlyph3DMapperMasking.cxx
  TestGlyph3DMapperOrientationArray.cxx
  TestGlyph3DMapperPicking.cxx
  TestGlyph3DMapperQuaternionArray.cxx
  TestGlyph3DMapperTreeIndexing.cxx,NO_DATA
  TestGradientBackground.cxx
  TestGradientBackgroundWithTiledViewport.cxx,NO_DATA
  TestGradientBackgroundWithTiledViewports.cxx,NO_DATA
# TestHardwarePicker.cxx,NO_DATA
  TestHardwareSelector.cxx,NO_DATA,NO_VALID
  TestHomogeneousTransformOfActor.cxx
  TestImageAndAnnotations.cxx,NO_DATA
  TestInteractorStyleImageProperty.cxx,NO_VALID
  TestLabeledContourMapper.cxx
  TestLabeledContourMapperNoLabels.cxx
  TestLabeledContourMapperWithActorMatrix.cxx
  TestLineRendering.cxx,NO_DATA
  TestManyActors.cxx,NO_VALID
  TestManyRenderWindows.cxx,NO_VALID,NO_RT,NO_DATA
  TestMapVectorsAsRGBColors.cxx
  TestMapVectorsToColors.cxx
  TestMixedGeometryCellScalars.cxx,NO_DATA
  TestNActorsNMappersOneInput.cxx,NO_DATA
  TestNActorsOneMapper.cxx,NO_DATA
  TestNViewportsNActorsNMappersNInputs.cxx,NO_DATA
  TestNViewportsNActorsNMappersOneInput.cxx,NO_DATA
  TestNViewportsNActorsOneMapper.cxx,NO_DATA
  TestNViewportsOneActor.cxx,NO_DATA
  TestOffAxisStereo.cxx
  TestOnAndOffScreenConeCxx.cxx
  TestOpacity.cxx
  TestOpacityMSAA.cxx
  TestOrderedTriangulator.cxx
  TestOSConeCxx.cxx
  TestPickingManager.cxx,NO_VALID
  TestPickTextActor.cxx,NO_VALID
  TestPointPicker.cxx,NO_DATA
  TestPointScalarMappedColors.cxx,NO_DATA
  TestPointSelection.cxx,NO_VALID
  TestPointSelectionWithCellData.cxx,NO_VALID
  TestPolyDataMapper2D.cxx,NO_DATA
  TestPolyDataMapperClipPlanes.cxx,NO_DATA
  TestPolyDataMapperNormals.cxx
  TestPolygonSelection.cxx,NO_DATA
  TestQuad.cxx,NO_DATA
  TestQuadPointRep.cxx,NO_DATA
  TestReadPixels.cxx,NO_DATA,NO_VALID
  TestRemoveActors.cxx,NO_DATA,NO_VALID
  TestRenderLinesAsTubes.cxx
  TestRenderPointsAsSpheres.cxx,NO_DATA
  TestResetCameraScreenSpace.cxx
  TestResetCameraVerticalAspectRatio.cxx
  TestResetCameraVerticalAspectRatioParallel.cxx
  TestResizingWindowToImageFilter.cxx,NO_DATA
  TestScalarModeToggle.cxx,NO_DATA
  TestSelectVisiblePoints.cxx,NO_VALID
  TestSplitViewportStereoHorizontal.cxx
  TestStereoBackgroundLeft.cxx
  TestStereoBackgroundRight.cxx
  TestStereoEyeSeparation.cxx
  TestSurfacePlusEdges.cxx,NO_DATA
  TestTexturedBackground.cxx
  TestTexturedCylinder.cxx
  TestTextureInterpolateScalars.cxx
  TestTextureRGBA.cxx
  TestTextureRGBADepthPeeling.cxx
  TestTextureSize.cxx,NO_VALID
  TestTextureWrap.cxx
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
  TestVertexRendering.cxx,NO_DATA
  TestVertexVisibility.cxx
  TestWindowToImageFilter.cxx
  TestWireframe.cxx,NO_DATA
  TestWorldPointPicker.cxx,NO_DATA
  # TestViewDependentErrorMetric.cxx # Loads of extra deps - move?
  # TestHierarchicalBoxPipeline.cxx  # legacy code
  )
  
  set(TestLineRenderingTranslucent_ARGS --translucent)
  list(APPEND cxx_tests TestLineRenderingTranslucent,TestLineRendering.cxx,NO_DATA)

  set(TestMixedGeometry_1_ARGS --point-size 0.5 --line-width 0.5)
  list(APPEND cxx_tests TestMixedGeometry_1,TestMixedGeometry.cxx,NO_DATA)
  
  set(TestMixedGeometry_2_ARGS --point-size 1.0 --line-width 1.0)
  list(APPEND cxx_tests TestMixedGeometry_2,TestMixedGeometry.cxx,NO_DATA)
  
  set(TestMixedGeometry_3_ARGS --point-size 3.0 --line-width 3.0)
  list(APPEND cxx_tests TestMixedGeometry_3,TestMixedGeometry.cxx,NO_DATA)
  
  set(TestPointRendering_1_ARGS --point-size 0.5)
  list(APPEND cxx_tests TestPointRendering_1,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRendering_2_ARGS --point-size 1.0)
  list(APPEND cxx_tests TestPointRendering_2,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRendering_3_ARGS --point-size 3.0)
  list(APPEND cxx_tests TestPointRendering_3,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRendering_4_ARGS --point-size 7.0)
  list(APPEND cxx_tests TestPointRendering_4,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRenderingRound_1_ARGS --point-size 0.5 --round)
  list(APPEND cxx_tests TestPointRenderingRound_1,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRenderingRound_2_ARGS --point-size 1.0 --round)
  list(APPEND cxx_tests TestPointRenderingRound_2,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRenderingRound_3_ARGS --point-size 3.0 --round)
  list(APPEND cxx_tests TestPointRenderingRound_3,TestPointRendering.cxx,NO_DATA)
  
  set(TestPointRenderingRound_4_ARGS --point-size 7.0 --round)
  list(APPEND cxx_tests TestPointRenderingRound_4,TestPointRendering.cxx,NO_DATA)
  
  set(TestPolyDataMapper2DPointScalarColorMapping_ARGS --point-colors)
  list(APPEND cxx_tests TestPolyDataMapper2DPointScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA)
  
  set(TestPolyDataMapper2DCellScalarColorMapping_ARGS --cell-colors)
  list(APPEND cxx_tests TestPolyDataMapper2DCellScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA)
  
  set(TestRenderPointsAsSpheresOrthoCamera_ARGS --ortho)
  list(APPEND cxx_tests TestRenderPointsAsSpheresOrthoCamera,TestRenderPointsAsSpheres.cxx,NO_DATA)
  
  set(TestRenderLinesAsTubesOrthoCamera_ARGS --ortho)
  list(APPEND cxx_tests TestRenderLinesAsTubesOrthoCamera,TestRenderLinesAsTubes.cxx,NO_DATA)
  
if (TARGET VTK::RenderingOpenGL2)
  set(vtk_test_prefix "OpenGL-")
  vtk_add_test_cxx(vtkRenderingCoreCxxTests tests
    --vtk-factory-prefer RenderingBackend=OpenGL
    ${extra_opengl2_tests}
    ${cxx_tests})
  unset(vtk_test_prefix)
  # Below, we set some special properties on OpenGL tests
  if (VTK_USE_X)
    set_tests_properties(VTK::RenderingCoreCxx-OpenGL-TestInteractorTimers
      PROPERTIES RUN_SERIAL ON)
  endif ()
  # Extend the timeout because this test can take much longer on slow machines.
  if (NOT VTK_TEST_TIMEOUT_TestManyRenderWindows)
    set(VTK_TEST_TIMEOUT_TestManyRenderWindows 300)
  endif()
  set_tests_properties(VTK::RenderingCoreCxx-OpenGL-TestManyRenderWindows
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestManyRenderWindows})
endif ()

if (TARGET VTK::RenderingWebGPU)
  set(vtk_test_prefix "WebGPU-")
  vtk_add_test_cxx(vtkRenderingCoreCxxTests tests
    --vtk-factory-prefer RenderingBackend=WebGPU
    ${cxx_tests})
  unset(vtk_test_prefix)
  # Extend the timeout because this test can take much longer on slow machines.
  if (NOT VTK_TEST_TIMEOUT_TestManyRenderWindows)
    set(VTK_TEST_TIMEOUT_TestManyRenderWindows 300)
  endif()
  set_tests_properties(VTK::RenderingCoreCxx-WebGPU-TestManyRenderWindows
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestManyRenderWindows})
endif ()

vtk_test_cxx_executable(vtkRenderingCoreCxxTests tests)

if (TARGET VTK::RenderingWebGPU)
  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Enable JSPI for webgpu async API
    target_link_options(vtkRenderingCoreCxxTests
      PRIVATE
        "-Wno-experimental"
        "-sJSPI=1"
        "-sJSPI_EXPORTS=['__wasm_call_ctors']")
  endif ()
endif ()
