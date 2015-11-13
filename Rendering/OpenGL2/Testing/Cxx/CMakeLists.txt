vtk_add_test_cxx(${vtk-module}CxxTests tests
  #TestRenderWidget.cxx # Very experimental, fails, does nothing useful yet.
  TestDepthOfFieldPass.cxx
  TestLightingMapLuminancePass.cxx
  TestLightingMapNormalsPass.cxx
  TestPointGaussianMapper.cxx
  TestPointGaussianMapperOpacity.cxx
  TestUserShader.cxx
  TestUserShader2.cxx
  TestValuePass.cxx
  TestValuePass2.cxx
  TestVBOPLYMapper.cxx
  TestVBOPointsLines.cxx
  TestGaussianBlurPass.cxx
  TestSSAAPass.cxx
  TestBlurAndSobelPasses.cxx
  TestShadowMapBakerPass.cxx
  TestShadowMapPass.cxx
  TestSobelGradientMagnitudePass.cxx
  TestEDLPass.cxx
  TestCoincident.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
