SET(RenderingTests
  otherCoordinate.cxx
  TestPriorityStreaming.cxx
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# For tests that actually use a vtkRenderWindow
SET(RenderingTestsWithArguments
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
  TestInteractorStyleImageProperty.cxx
  TestInteractorTimers.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  TestLightActor.cxx
  TestLODActor.cxx
  TestManyActors.cxx
  TestOffAxisStereo.cxx
  TestOrderedTriangulator.cxx
  TestOpacity.cxx
  TestOpenGLPolyDataMapper.cxx
  TestOSConeCxx.cxx
  TestPolygonSelection.cxx
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

if(APPLE)
  list(REMOVE_ITEM RenderingTestsWithArguments TestShadowMapPass.cxx)
endif()

if(WIN32 AND NOT VTK_USE_X)
  set(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    TestWin32OpenGLRenderWindow.cxx
    )
endif()

include_directories(${OPENGL_INCLUDE_DIR})

if(VTK_DATA_ROOT)
  set(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    TestAreaSelections.cxx
    TestGlyph3DMapperArrow.cxx
    TestMultiblockDisplayProperties.cxx
    TestMultiTexturing.cxx
    TestMultiTexturingTransform.cxx
    TestImageSliceMapperBorder.cxx
    TestImageResliceMapperBorder.cxx
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
endif()

if(VTK_USE_DISPLAY AND VTK_DATA_ROOT)
  set(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    RenderNonFinite.cxx
    SurfacePlusEdges.cxx
    TestLabelPlacementMapper.cxx
    )
endif()

set(VTK_USE_GLSL_SHADERS TRUE)
if(VTK_USE_GLSL_SHADERS)
  # Tests testing GLSL Shaders.
  set(RenderingTestsWithArguments
    ${RenderingTestsWithArguments}
    TestGenericVertexAttributesGLSLCxx.cxx
    TestGenericVertexAttributesGLSLAlphaBlending.cxx
    TestGenericVertexAttributesGLSLDepthPeelingPass.cxx
    )
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${RenderingTests};${RenderingTestsWithArguments}
  EXTRA_INCLUDE vtkTestDriver.h
  )

set(TEST_FBO_IMPLEMENTATION_EXE
  TestFBOImplementation)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestFBOInclude.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/TestFBOInclude.h
  @ONLY IMMEDIATE ESCAPE_QUOTES)

# This is used by TestFBO. Using a separate processes makes is possible to
# avoid issues to OpenGL implementations segfaulting etc. from affecting the
# test.
vtk_module_test_executable(TestFBOImplementation
  TestFBOImplementation.cxx)
target_link_libraries(TestFBOImplementation
  vtkRenderingOpenGL ${OPENGL_gl_LIBRARY})

if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  target_link_libraries(TestFBOImplementation ${OSMESA_LIBRARY})
endif()

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  target_link_libraries(${vtk-module}CxxTests ${OSMESA_LIBRARY})
endif()

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${RenderingTests})
  get_filename_component(TName ${test} NAME_WE)
  add_test(NAME ${vtk-module}Cxx-${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
      -T ${VTK_TEST_OUTPUT_DIR}
    )
endforeach()

foreach(test ${RenderingTestsWithArguments})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -T ${VTK_TEST_OUTPUT_DIR}
      )
  endif()
endforeach()

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
  PROPERTIES TIMEOUT 90)
