# add tests that do not require data or produce vector output
set(TestFreeTypeTextMapperNoMath_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
set(TestMathTextFreeTypeTextRendererNoMath_ARGS
  DATA{../Data/Fonts/DejaVuSans.ttf}
)
vtk_add_test_cxx(vtkRenderingFreeTypeCxxTests tests
  TestBillboardTextActor3D.cxx
  TestFTStringToPath.cxx
  TestFlagpoleLabel.cxx
  TestFreeTypeTextMapperNoMath.cxx
  TestFreeTypeTools.cxx,NO_VALID
  TestMathTextFreeTypeTextRendererNoMath.cxx
  TestTextActor.cxx
  TestTextActor3D.cxx
  TestTextActorAlphaBlending.cxx
  TestTextActorDepthPeeling.cxx
  TestTextActor3DAlphaBlending.cxx
  TestTextActor3DDepthPeeling.cxx
  TestTextActorScaleModeProp.cxx
  TestTextMapper.cxx
  TestTextOpaquePass.cxx
  )

vtk_add_test_cxx(vtkRenderingFreeTypeCxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
  TestTextBoundingBox.cxx
  )
list(APPEND tests
  ${no_data_tests})

# Check for vtkErrorMacro calls in these tests (mainly for empty inputs):
set_tests_properties(
  VTK::RenderingFreeTypeCxx-TestTextActor
  VTK::RenderingFreeTypeCxx-TestTextActor3D
  PROPERTIES FAIL_REGULAR_EXPRESSION "ERROR:"
)

if(TARGET VTK::RenderingMatplotlib)
  set(TestMathTextFreeTypeTextRenderer_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
  set(TestFreeTypeTextMapper_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
  set(TestFontDPIScaling_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})

  vtk_add_test_cxx(vtkRenderingFreeTypeCxxTests matplotlib_tests
    TestMathTextFreeTypeTextRenderer.cxx
    TestFreeTypeTextMapper.cxx
    TestFontDPIScaling.cxx
    )
  list(APPEND tests
    ${matplotlib_tests})
endif()

vtk_test_cxx_executable(vtkRenderingFreeTypeCxxTests tests
  RENDERING_FACTORY)
