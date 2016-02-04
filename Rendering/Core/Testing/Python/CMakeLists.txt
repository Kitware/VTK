if(VTK_USE_GLSL_SHADERS)
  set(glsl_tests
    TestGLSLShader.py,NO_VALID
    )
endif()

vtk_add_test_python(
  CamBlur.py
  ImageActor.py
  ImageActorStressed.py
  NoLightGeneration.py
  PolyDataMapperAllPolygons.py
  PolyDataMapperAllWireframe.py
  ScalarBar.py
  TestCameraInterpolator.py
  TestMapperLUT.py,NO_VALID
  TestOnePoint.py
  # TestParallelCoordinates.py) # disabled in TCL too
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
  pickCells.py
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
  TestScalarMaterialMode.py
  cells.py,NO_RT
  ${glsl_tests}
  )

if(NOT VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
vtk_add_test_python(
  # pass triangle cell scalars into a strip as field data
  # have the renderer render the triangle strip using the
  # original triangle cell data (now stored in field data)
  # for the opengl2 backend we are not supporting this
  # as strips are not recommended and this is an odd case
  # of trying to get higher performance from strips but yet
  # still trying to make them render as individual triangles
  # for opengl2 just skip the stripper and the result will
  # be what you want
  TestTriangleStripCellColor.py
)
endif()

vtk_add_test_python(
  TestOutOfRangeDiscretizableColorTransferFunction.py --useBelowRangeColor --useAboveRangeColor
)

vtk_add_test_python(
  TestOutOfRangeColorTransferFunction.py --useBelowRangeColor --useAboveRangeColor
)

# Tests that do not use a baseline image
vtk_add_test_python(
  NO_DATA NO_VALID NO_OUTPUT
  pickCellPoints.py
)
