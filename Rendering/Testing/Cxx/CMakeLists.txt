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
    TestBlurAndSobelPasses.cxx
    TestDynamic2DLabelMapper.cxx
    TestFBO.cxx
    TestGaussianBlurPass.cxx
    TestGPUInfo.cxx
    TestGradientBackground.cxx
    TestInteractorTimers.cxx
    TestLabelPlacer.cxx
    TestLabelPlacer2D.cxx
    TestLabelPlacerCoincidentPoints.cxx
    TestLabelPlacementMapper2D.cxx
    TestLabelPlacementMapperCoincidentPoints.cxx
    TestLightActor.cxx
    TestLODActor.cxx
    TestManyActors.cxx
    TestOrderedTriangulator.cxx
    TestOpacity.cxx
    TestOSConeCxx.cxx
    TestPOVExporter.cxx
    TestResetCameraVerticalAspectRatio.cxx
    TestResetCameraVerticalAspectRatioParallel.cxx
    TestSobelGradientMagnitudePass.cxx
    TestShadowMapPass.cxx
    TestTextActorAlphaBlending.cxx
    TestTextActorDepthPeeling.cxx
    TestTextActor3DAlphaBlending.cxx
    TestTextActor3DDepthPeeling.cxx
    TestTexturedBackground.cxx
    TestTDx.cxx
    TestTilingCxx.cxx
    TestTranslucentLUTAlphaBlending.cxx
    TestTranslucentLUTDepthPeeling.cxx
    TestTranslucentLUTDepthPeelingPass.cxx
    TestTranslucentLUTTextureAlphaBlending.cxx
    TestTranslucentLUTTextureDepthPeeling.cxx
    )

  IF(VTK_DATA_ROOT)
    SET(RenderingTestsWithArguments
      ${RenderingTestsWithArguments}
      TestAreaSelections.cxx
      TestMultiTexturing.cxx
      TestMultiTexturingTransform.cxx
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
    # Temporarily disable LIC tests on non-windows platforms
    IF(WIN32)
      IF (VTK_DATA_ROOT)
        SET( RenderingTestsWithArguments
        ${RenderingTestsWithArguments}
        TestSurfaceLIC.cxx
        )
      ENDIF (VTK_DATA_ROOT)
    ENDIF(WIN32)
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

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO ${OPENGL_gl_LIBRARY})


SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

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
ENDFOREACH (exe)

IF(VTK_USE_GLSL_SHADERS AND VTK_USE_DISPLAY)
  set_tests_properties(TestGenericVertexAttributesGLSLDepthPeelingPass PROPERTIES FAIL_REGULAR_EXPRESSION "ERROR:.*vtkOpenGLProperty.*Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.")
ENDIF(VTK_USE_GLSL_SHADERS AND VTK_USE_DISPLAY)
