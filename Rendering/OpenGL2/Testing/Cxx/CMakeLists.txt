vtk_module_test_data(
  Data/pen_1.cur
  Data/pen_1.xcursor
  Data/overlap_faces.vtm
  Data/overlap_faces/overlap_faces_0_0.vtp
  Data/overlap_faces/overlap_faces_1_0.vtp
  Data/overlap_faces/overlap_faces_2_0.vtp
  )

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()
vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
  TestBlockVisibility.cxx
  TestBlurAndSobelPasses.cxx
  TestCameraShiftScale.cxx,NO_DATA
  TestCoincident.cxx
  TestCoincidentFloor.cxx
  TestCompositeDataOverlappingCells.cxx,
  TestCompositeDataPointGaussian.cxx,NO_DATA
  TestCompositeDataPointGaussianSelection.cxx,NO_DATA
  TestComputeShader.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestCubeMap.cxx
  TestCubeMap2.cxx
  TestCubeMapRerender.cxx
  TestCustomCursorShape.cxx,NO_VALID
  TestDepthOfFieldPass.cxx
  TestDepthPeelingOcclusionQuery.cxx
  TestDepthPeelingPass.cxx
  TestDepthPeelingPassViewport.cxx
  TestEDLPass.cxx
  TestEquirectangularToCubeMap.cxx
  TestFXAAFilter.cxx
  TestFXAAPass.cxx
  TestFXAAWithEDLPass.cxx
  TestFlipRenderFramebuffer.cxx
  TestFloor.cxx,LOOSE_VALID
  TestFluidMapper.cxx
  TestFramebufferHDR.cxx
  TestFramebufferPass.cxx
  TestGaussianBlurPass.cxx
  TestGlyph3DMapperCellPicking.cxx
  TestGlyph3DMapperCulling.cxx
  TestGlyph3DMapperEdges.cxx
  TestGlyph3DMapperPickability.cxx,NO_DATA
  TestGlyph3DMapperTreeIndexingCompositeGlyphs.cxx,NO_DATA
  TestHiddenLineRemovalPass.cxx
  TestLightingMapLuminancePass.cxx
  TestLightingMapNormalsPass.cxx
  TestMultiTexturing.cxx
  TestMultiTexturingInterpolateScalars.cxx
  TestNormalMapping.cxx
  TestNormalMappingWithEdges.cxx
  TestOffscreenRenderingResize.cxx
  TestOutlineGlowPass.cxx
  TestOrderIndependentTranslucentPass.cxx
  TestPanoramicProjectionPass.cxx,NO_DATA
  TestPBRClearCoat.cxx
  TestPBREdgeTint.cxx
  TestPBRHdrEnvironment.cxx
  TestPBRAnisotropy.cxx
  TestPBRColorMultiplier.cxx
  TestPBRIrradianceHDR.cxx
  TestPBRMapping.cxx
  TestPBRMaterials.cxx
  TestPBRMaterialsCoat.cxx
  TestPBRTextureInterpolateScalars.cxx
  TestPointFillPass.cxx
  TestPointGaussianMapper.cxx
  TestPointGaussianMapperAnisotropic.cxx
  TestPointGaussianMapperOpacity.cxx
  TestPointGaussianSelection.cxx,NO_DATA
  TestProgramPointSize.cxx
  TestPropPicker2Renderers.cxx,NO_DATA
  TestRemoveActorNonCurrentContext.cxx
  TestRenderToImage.cxx
  TestSetZBuffer.cxx
  TestShadowMapBakerPass.cxx
  TestShadowMapPass.cxx
  TestSharedRenderWindow.cxx
  TestDirectSelectionRendering.cxx
  TestSimpleMotionBlur.cxx
  TestSobelGradientMagnitudePass.cxx
  TestSphereMap.cxx
  TestSpherePoints.cxx
  TestSphereVertex.cxx
  TestSRGB.cxx
  TestSSAAPass.cxx
  TestSSAOPass.cxx
  TestSSAOPassWithRenderer.cxx
  TestSurfaceInterpolationSwitch.cxx
  TestTessellationShader.cxx,NO_DATA
  TestTexture16Bits.cxx,NO_DATA
  TestTexture32Bits.cxx,NO_DATA
  TestTextureBufferEmulation.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestTextureInterpolateScalars.cxx
  TestToneMappingPass.cxx,NO_DATA
  TestUserShader.cxx
  TestUserShader2.cxx
  TestUserShader2D.cxx,NO_DATA
  TestValuePassFloatingPoint.cxx
  TestValuePassFloatingPoint2.cxx
  TestVBOPLYMapper.cxx
  TestVBOPointsLines.cxx
  TestWindowBlits.cxx
  UnitTestOpenGLUniforms.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TestFramebufferHDR)
  set(VTK_TEST_TIMEOUT_TestFramebufferHDR 300)
endif()
set_tests_properties(VTK::RenderingOpenGL2Cxx-TestFramebufferHDR
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestFramebufferHDR})

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  if (VTK_USE_SDL2)
    vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests TestSDL2.cxx)
  endif ()
endif ()

if (NOT VTK_OPENGL_USE_GLES)
  # vtkFastLabeledDataMapper needs geometry shaders and GLES does not have them.
  vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
    TestLabeledDataMappers.cxx)
endif ()

if (VTK_USE_WIN32_OPENGL)
  vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
    TestWin32OpenGLDXRenderWindow.cxx)
endif ()

vtk_test_cxx_executable(vtkRenderingOpenGL2CxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )

if (VTK_USE_WIN32_OPENGL)
  # TestWin32OpenGLDXRenderWindow requires DirectX
  find_package(DirectX REQUIRED)
  target_link_libraries(vtkRenderingOpenGL2CxxTests PRIVATE DirectX::d3d11 DirectX::dxgi)
endif ()
