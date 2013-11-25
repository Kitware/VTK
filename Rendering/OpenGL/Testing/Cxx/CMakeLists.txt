vtk_add_test_cxx(NO_DATA NO_VALID
  otherCoordinate.cxx
  TestPriorityStreaming.cxx
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# For tests that actually use a vtkRenderWindow
vtk_add_test_cxx(
  LoadOpenGLExtension.cxx
  TestActorLightingFlag.cxx
  TestAnimationScene.cxx
  TestBackfaceCulling.cxx
  TestBlurAndSobelPasses.cxx
  TestDynamic2DLabelMapper.cxx
  TestFBO.cxx,NO_VALID
  TestFollowerPicking.cxx
  TestGaussianBlurPass.cxx
  TestGlyph3DMapper.cxx
  TestGlyph3DMapperMasking.cxx
  TestGlyph3DMapperOrientationArray.cxx
  TestGlyph3DMapperPicking.cxx
  TestGPUInfo.cxx,NO_VALID
  TestGradientBackground.cxx
  TestHomogeneousTransformOfActor.cxx
  TestImageResliceMapperAlpha.cxx
  TestImageResliceMapperBackground.cxx
  TestImageResliceMapperInterpolation.cxx
  TestImageResliceMapperOffAxis.cxx
  TestImageResliceMapperOrient3D.cxx
  TestImageResliceMapperSlab.cxx
  TestImageSliceMapperAlpha.cxx
  TestImageSliceMapperBackground.cxx
  TestImageSliceMapperOrient2D.cxx
  TestImageSliceMapperOrient3D.cxx
  TestImageSliceMapperInterpolation.cxx
  TestImageStack.cxx
  TestInteractorStyleImageProperty.cxx,NO_VALID
  TestInteractorTimers.cxx,NO_VALID
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  TestLightActor.cxx
  TestLODActor.cxx,NO_VALID
  TestManyActors.cxx,NO_VALID
  TestMonitors.cxx,NO_VALID
  TestOffAxisStereo.cxx
  TestOrderedTriangulator.cxx
  TestOpacity.cxx
  TestOpenGLPolyDataMapper.cxx
  TestOSConeCxx.cxx
  TestPolygonSelection.cxx
  TestPOVExporter.cxx,NO_VALID
  TestResetCameraVerticalAspectRatio.cxx
  TestResetCameraVerticalAspectRatioParallel.cxx
  TestSetImageOrientation.cxx
  TestSobelGradientMagnitudePass.cxx
  TestTextActorAlphaBlending.cxx
  TestTextActorDepthPeeling.cxx
  TestTextActor3DAlphaBlending.cxx
  TestTextActor3DDepthPeeling.cxx
  TestTexturedBackground.cxx
  TestTextureSize.cxx,NO_VALID
  TestTDx.cxx
  TestTilingCxx.cxx
  TestTransformCoordinateUseDouble.cxx
  TestTranslucentLUTAlphaBlending.cxx
  TestTranslucentLUTDepthPeeling.cxx
  TestTranslucentLUTDepthPeelingPass.cxx
  TestTranslucentLUTTextureAlphaBlending.cxx
  TestTranslucentLUTTextureDepthPeeling.cxx
  )
if(NOT APPLE)
  vtk_add_test_cxx(TestShadowMapPass.cxx)
endif()

if(WIN32 AND NOT VTK_USE_X)
  vtk_add_test_cxx(TestWin32OpenGLRenderWindow.cxx NO_VALID)
endif()

include_directories(${OPENGL_INCLUDE_DIR})

vtk_add_test_cxx(
  TestAreaSelections.cxx
  TestGlyph3DMapperArrow.cxx
  TestMultiblockDisplayProperties.cxx
  TestMultiTexturing.cxx
  TestMultiTexturingTransform.cxx
  TestImageSliceMapperBorder.cxx
  TestImageResliceMapperBorder.cxx
  TestScalarBar.cxx
  TestScenePicker.cxx,NO_VALID
  TestTextureRGBA.cxx
  TestTextureRGBADepthPeeling.cxx
  TestTranslucentImageActorAlphaBlending.cxx
  TestTranslucentImageActorDepthPeeling.cxx
  TestTStripsColorsTCoords.cxx
  TestTStripsNormalsColorsTCoords.cxx
  TestTStripsNormalsTCoords.cxx
  TestTStripsTCoords.cxx
  RenderNonFinite.cxx
  SurfacePlusEdges.cxx
  TestLabelPlacementMapper.cxx
  )

set(VTK_USE_GLSL_SHADERS FALSE)
if(VTK_USE_GLSL_SHADERS)
  # Tests testing GLSL Shaders.
  vtk_add_test_cxx(
    TestGenericVertexAttributesGLSLCxx.cxx
    TestGenericVertexAttributesGLSLAlphaBlending.cxx
    TestGenericVertexAttributesGLSLDepthPeelingPass.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)

set(TEST_FBO_IMPLEMENTATION_EXE
  TestFBOImplementation)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestFBOInclude.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/TestFBOInclude.h
  @ONLY ESCAPE_QUOTES)

# This is used by TestFBO. Using a separate processes makes is possible to
# avoid issues to OpenGL implementations segfaulting etc. from affecting the
# test.
vtk_module_test_executable(TestFBOImplementation
  TestFBOImplementation.cxx)
target_link_libraries(TestFBOImplementation LINK_PRIVATE
  vtkRenderingOpenGL ${OPENGL_gl_LIBRARY})

if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  target_link_libraries(TestFBOImplementation LINK_PRIVATE ${OSMESA_LIBRARY})
endif()

if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE ${OSMESA_LIBRARY})
endif()

# Add other odd tests or executables
foreach(exe
  TimeRenderer
  TimeRenderer2
  VTKBenchMark
  )
  add_executable(${exe} ${exe}.cxx)
  target_link_libraries(${exe}
    vtkRenderingOpenGL
    vtkRenderingFreeTypeOpenGL
    vtkInteractionStyle
    vtkImagingSources
    vtkImagingGeneral
    vtkFiltersSources
    vtkIOCore
    ${OPENGL_gl_LIBRARY})
  if(APPLE)
    target_link_libraries(${exe} "-framework GLUT")
  endif()
  if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
    target_link_libraries(${exe} ${OSMESA_LIBRARY})
  endif()
endforeach()

if(VTK_USE_GLSL_SHADERS)
  set_tests_properties(
    ${vtk-module}Cxx-TestGenericVertexAttributesGLSLDepthPeelingPass
    PROPERTIES FAIL_REGULAR_EXPRESSION "ERROR:.*vtkOpenGLProperty.*Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.")
endif()

set_tests_properties(${vtk-module}Cxx-TestOpacity
  PROPERTIES TIMEOUT 70)
set_tests_properties(${vtk-module}Cxx-TestTextureSize
  PROPERTIES TIMEOUT 300)
