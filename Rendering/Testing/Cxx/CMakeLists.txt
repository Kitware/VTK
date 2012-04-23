SET(KIT Rendering)

SET(RenderingTests
  otherCoordinate.cxx
  TestPriorityStreaming.cxx
  )

SET(RenderingTestsWithArguments)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(VTK_USE_DISPLAY)
  # For tests that actually use a vtkRenderWindow
  SET(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    LoadOpenGLExtension.cxx
    TestActorLightingFlag.cxx
    TestAnimationScene.cxx
    TestBackfaceCulling.cxx
    TestBlurAndSobelPasses.cxx
    TestDynamic2DLabelMapper.cxx
    TestFBO.cxx
    TestFollowerPicking.cxx
    TestGaussianBlurPass.cxx
    TestGlyph3DMapper.cxx
    TestGlyph3DMapperMasking.cxx
    TestGlyph3DMapperOrientationArray.cxx
    TestGlyph3DMapperPicking.cxx
    TestGPUInfo.cxx
    TestGradientBackground.cxx
    TestHomogeneousTransformOfActor.cxx
    TestImageResliceMapperAlpha.cxx
    TestImageResliceMapperBackground.cxx
    TestImageResliceMapperBorder.cxx
    TestImageResliceMapperInterpolation.cxx
    TestImageResliceMapperOffAxis.cxx
    TestImageResliceMapperOrient3D.cxx
    TestImageResliceMapperSlab.cxx
    TestImageSliceMapperAlpha.cxx
    TestImageSliceMapperBackground.cxx
    TestImageSliceMapperBorder.cxx
    TestImageSliceMapperOrient2D.cxx
    TestImageSliceMapperOrient3D.cxx
    TestImageSliceMapperInterpolation.cxx
    TestImageStack.cxx
    TestInteractorTimers.cxx
    TestLabelPlacer.cxx
    TestLabelPlacer2D.cxx
    TestLabelPlacerCoincidentPoints.cxx
    TestLabelPlacementMapper2D.cxx
    TestLabelPlacementMapperCoincidentPoints.cxx
    TestLightActor.cxx
    TestLODActor.cxx
    TestManyActors.cxx
#    TestOffAxisStereo.cxx
    TestOrderedTriangulator.cxx
    TestOpacity.cxx
    TestOpenGLPolyDataMapper.cxx
    TestOSConeCxx.cxx
    TestPOVExporter.cxx
    TestResetCameraVerticalAspectRatio.cxx
    TestResetCameraVerticalAspectRatioParallel.cxx
    TestSetImageOrientation.cxx
    TestSobelGradientMagnitudePass.cxx
    TestShadowMapPass.cxx
    TestTextActorAlphaBlending.cxx
    TestTextActorDepthPeeling.cxx
    TestTextActor3DAlphaBlending.cxx
    TestTextActor3DDepthPeeling.cxx
    TestTexturedBackground.cxx
    TestTextureSize.cxx
    TestTDx.cxx
    TestTilingCxx.cxx
    TestTransformCoordinateUseDouble.cxx
    TestTranslucentLUTAlphaBlending.cxx
    TestTranslucentLUTDepthPeeling.cxx
    TestTranslucentLUTDepthPeelingPass.cxx
    TestTranslucentLUTTextureAlphaBlending.cxx
    TestTranslucentLUTTextureDepthPeeling.cxx
    )

  IF(WIN32 AND NOT VTK_USE_X)
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestWin32OpenGLRenderWindow.cxx
      )
  ENDIF(WIN32 AND NOT VTK_USE_X)

  IF(VTK_DATA_ROOT)
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestAreaSelections.cxx
      TestMultiTexturing.cxx
      TestMultiTexturingTransform.cxx
      TestGlyph3DMapperArrow.cxx
      TestScalarBar.cxx
      TestScenePicker.cxx
      TestTextureRGBA.cxx
      TestTextureRGBADepthPeeling.cxx
      TestTranslucentImageActorAlphaBlending.cxx
      TestTranslucentImageActorDepthPeeling.cxx
      TestTStripsColorsTCoords.cxx
      TestTStripsNormalsColorsTCoords.cxx
      TestTStripsNormalsTCoords.cxx
      TestTStripsTCoords.cxx
      )
  ENDIF(VTK_DATA_ROOT)

  IF(VTK_USE_DISPLAY AND VTK_DATA_ROOT)
  SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      RenderNonFinite.cxx
      SurfacePlusEdges.cxx
      TestLabelPlacementMapper.cxx
      )
  ENDIF(VTK_USE_DISPLAY AND VTK_DATA_ROOT)

  IF (VTK_USE_GLSL_SHADERS)
    # Tests testing GLSL Shaders.
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestGenericVertexAttributesGLSLCxx.cxx
      TestGenericVertexAttributesGLSLAlphaBlending.cxx
      TestGenericVertexAttributesGLSLDepthPeelingPass.cxx
      )
    # Temporarily disable LIC tests on Mac
    IF(NOT APPLE)
      IF (VTK_DATA_ROOT)
        SET( RenderingTestsWithArguments
        ${RenderingTestsWithArguments}
        TestSurfaceLIC.cxx
        )
      ENDIF (VTK_DATA_ROOT)
    ENDIF()
  ENDIF (VTK_USE_GLSL_SHADERS)

ENDIF(VTK_USE_DISPLAY)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  ${RenderingTests};${RenderingTestsWithArguments}
  EXTRA_INCLUDE vtkTestDriver.h
  )

SET (TEST_FBO_IMPLEMENTATION_EXE
  TestFBOImplementation)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/TestFBOInclude.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/TestFBOInclude.h
  @ONLY IMMEDIATE ESCAPE_QUOTES)

# This is used by TestFBO. Using a separate processes makes is possible to
# avoid issues to OpenGL implementations segfaulting etc. from affecting the
# test.
ADD_EXECUTABLE(TestFBOImplementation
  TestFBOImplementation.cxx)
TARGET_LINK_LIBRARIES(TestFBOImplementation
  vtkRendering ${OPENGL_gl_LIBRARY})

IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  TARGET_LINK_LIBRARIES(TestFBOImplementation ${OSMESA_LIBRARY})
ENDIF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO ${OPENGL_gl_LIBRARY})

IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  TARGET_LINK_LIBRARIES(${KIT}CxxTests ${OSMESA_LIBRARY})
ENDIF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)

SET (TestsToRun ${Tests})
LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables
FOREACH (test ${RenderingTests})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
    -T ${VTK_BINARY_DIR}/Testing/Temporary
    )
ENDFOREACH (test)

FOREACH (test ${RenderingTestsWithArguments})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      )
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH(test)

#
# Add other odd tests or executables
#
FOREACH (exe
    TimeRenderer
    TimeRenderer2
    VTKBenchMark
    )
  ADD_EXECUTABLE(${exe} ${exe}.cxx)
  TARGET_LINK_LIBRARIES(${exe} vtkRendering vtkIO ${OPENGL_gl_LIBRARY})
  IF (APPLE)
    TARGET_LINK_LIBRARIES(${exe} "-framework GLUT")
  ENDIF (APPLE)
  IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
    TARGET_LINK_LIBRARIES(${exe} ${OSMESA_LIBRARY})
  ENDIF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
ENDFOREACH (exe)

IF(VTK_USE_GLSL_SHADERS AND VTK_USE_DISPLAY)
  set_tests_properties(TestGenericVertexAttributesGLSLDepthPeelingPass PROPERTIES FAIL_REGULAR_EXPRESSION "ERROR:.*vtkOpenGLProperty.*Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.")
ENDIF(VTK_USE_GLSL_SHADERS AND VTK_USE_DISPLAY)
