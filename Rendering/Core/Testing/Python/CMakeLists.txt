if(VTK_USE_GLSL_SHADERS)
  set(glsl_tests
    TestGLSLShader.py,NO_VALID
    )
endif()

vtk_add_test_python(
  ImageActor.py
  ImageActorStressed.py
  NoLightGeneration.py
  PolyDataMapperAllPolygons.py
  PolyDataMapperAllWireframe.py
  ScalarBar.py
  TestCameraInterpolator.py
  TestMapperLUT.py,NO_VALID
  TestOnePoint.py
  TestParallelCoordinates.py
  TestRotate.py
  TestTransformInterpolator.py
  TransformConcatenation.py
  TransformCoverage.py
  TransformImageData.py
  TransformPolyData.py
  assembly.py
  cameraWarpedCone.py
  cowHair.py
  cowHair2.py
  distanceToCamera.py
  pickCells.py
  compositePolyDataMapperBounds.py,NO_RT
  pickImageData.py,NO_RT
  propAssembly.py
  rendererSource.py
  rotations.py
  stereoDresdenMace.py
  PickerWithLocator.py
  SurfacePickerWithTexture.py
  # XML shader code removed, test needs rewriting for new API
  #TestGenericVertexAttributesGLSL.py
  TestLeaderActor2D.py
  TestOpacity2.py
  TestOpacityVectors.py
  TestTransformCoordinateSystems.py
  TestWindowToImageTransparency.py
  TexturedSphere.py
  cells.py,NO_RT
#  TestScalarMaterialMode.py
  pickCompositeData.py,NO_RT
  pickCompositeData2.py,NO_RT
  ${glsl_tests}
  )

vtk_add_test_python(
  TestOutOfRangeDiscretizableColorTransferFunction.py --useBelowRangeColor --useAboveRangeColor
)

vtk_add_test_python(
  TestOutOfRangeColorTransferFunction.py --useBelowRangeColor --useAboveRangeColor
)

vtk_add_test_python(
  TestSetTableOutOfRangeLookupTable.py --useBelowRangeColor --useAboveRangeColor
)

# Tests that do not use a baseline image
vtk_add_test_python(
  NO_DATA NO_VALID NO_OUTPUT
  pickCellPoints.py
  TestGetRangeColorTransferFunction.py
  TestGetRangeDiscretizableColorTransferFunction.py
  TestGetRangeLookupTable.py
)
