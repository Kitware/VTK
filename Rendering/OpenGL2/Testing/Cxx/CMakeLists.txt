vtk_module_test_data(
  Data/pen_1.cur
  Data/pen_1.xcursor
  )

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
  set(_proxy_to_pthread_flag "0")
  if (VTK_WEBASSEMBLY_THREADS)
    set(_proxy_to_pthread_flag "1")
  endif ()
  set(_vtk_test_emscripten_extra_linker_args
    # O2 or higher make wasm-opt terribly slow.
    # test executables take a minute to link.
    "-O1"
    # don't keep runtime alive after exit() is called.
    "-sEXIT_RUNTIME=1"
    # this flag runs main() on a pthread i.e, a web worker.
    # the primary reason we desire this is to make the unit test access
    # data files through synchronous XHR which is only posssible from web-worker.
    "-sPROXY_TO_PTHREAD=${_proxy_to_pthread_flag}"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sOFFSCREENCANVAS_SUPPORT=${_proxy_to_pthread_flag}")
endif ()
vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests
  TestBlockVisibility.cxx
  TestBlurAndSobelPasses.cxx
  TestCameraShiftScale.cxx,NO_DATA
  TestCoincident.cxx
  TestCoincidentFloor.cxx
  TestCompositeDataPointGaussian.cxx,NO_DATA
  TestCompositeDataPointGaussianSelection.cxx,NO_DATA
  TestCompositePolyDataMapper2.cxx,NO_DATA
  TestCompositePolyDataMapper2CameraShiftScale.cxx,NO_DATA
  TestCompositePolyDataMapper2CellScalars.cxx,NO_DATA
  TestCompositePolyDataMapper2CustomShader.cxx,NO_DATA
  TestCompositePolyDataMapper2MixedGeometryCellScalars.cxx,NO_DATA
  TestCompositePolyDataMapper2MixedGeometryEdges.cxx,NO_DATA
  TestCompositePolyDataMapper2NaNPartial.cxx,NO_DATA
  TestCompositePolyDataMapper2PartialFieldData.cxx,NO_DATA
  TestCompositePolyDataMapper2PartialPointData.cxx,NO_DATA
  TestCompositePolyDataMapper2Pickability.cxx,NO_DATA
  TestCompositePolyDataMapper2Picking.cxx,NO_DATA
  TestCompositePolyDataMapper2Scalars.cxx,NO_DATA
# TestCompositePolyDataMapper2ScalarsSurfaceOpacity.cxx,NO_DATA #19221
  TestCompositePolyDataMapper2SharedArray.cxx,NO_DATA
  TestCompositePolyDataMapper2Spheres.cxx,NO_DATA
  TestCompositePolyDataMapper2Vertices.cxx,NO_DATA
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
  TestFloor.cxx, LOOSE_VALID
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

if (VTK_USE_SDL2)
  vtk_add_test_cxx(vtkRenderingOpenGL2CxxTests tests TestSDL2.cxx)
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
