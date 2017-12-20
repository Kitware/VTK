vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
  TestAppleBug.cxx
  TestBlurAndSobelPasses.cxx
  TestCoincident.cxx
  TestCompositePolyDataMapper2NaNPartial.cxx,NO_DATA
  TestCompositePolyDataMapper2Pickability.cxx,NO_DATA
  TestCompositePolyDataMapper2Spheres.cxx,NO_DATA
  TestCompositePolyDataMapper2Vertices.cxx,NO_DATA
  TestCubeMap.cxx
  TestDepthOfFieldPass.cxx
  TestDepthPeelingPass.cxx
  TestDepthPeelingPassViewport.cxx
  TestEDLPass.cxx
  TestFramebufferPass.cxx
  TestFramebufferHDR.cxx
  TestFXAAFilter.cxx
  TestGaussianBlurPass.cxx
  TestGlyph3DMapperEdges.cxx
  TestGlyph3DMapperCellPicking.cxx
  TestGlyph3DMapperCulling.cxx
  TestGlyph3DMapperPickability.cxx,NO_DATA
  TestLightingMapLuminancePass.cxx
  TestLightingMapNormalsPass.cxx
  TestOffscreenRenderingResize.cxx
  TestPointFillPass.cxx
  TestPropPicker2Renderers.cxx,NO_DATA
  TestPointGaussianMapper.cxx
  TestPointGaussianMapperOpacity.cxx
  TestRemoveActorNonCurrentContext.cxx
  TestRenderToImage.cxx
  TestSRGB.cxx
  TestSSAAPass.cxx
  TestSetZBuffer.cxx
  TestShadowMapBakerPass.cxx
  TestShadowMapPass.cxx
  TestSimpleMotionBlur.cxx
  TestSobelGradientMagnitudePass.cxx
  TestSpherePoints.cxx
  TestSphereVertex.cxx
  TestUserShader.cxx
  TestUserShader2.cxx
  TestVBOPLYMapper.cxx
  TestVBOPointsLines.cxx
  TestValuePass.cxx
  TestValuePass2.cxx
  )

vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
  TestCubeMap2.cxx
    "DATA{${VTK_TEST_INPUT_DIR}/skybox/,REGEX:.*}")

vtk_test_cxx_executable(vtkRenderingOpenGL2CxxTests tests RENDERING_FACTORY)
