INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# For tests that actually use a vtkRenderWindow
vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
  LoadOpenGLExtension.cxx
  TestFBO.cxx,NO_VALID
  TestGPUInfo.cxx,NO_VALID
  TestMonitors.cxx,NO_VALID
  TestPOVExporter.cxx,NO_VALID
  TestScalarsToColorsPainter.cxx
  TestSetImageOrientation.cxx
  TestTDx.cxx
  TestValuePainter.cxx,NO_VALID
  TestValuePasses.cxx
  )
vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
  TestScalarsToColorsPainterFieldDataCells,TestScalarsToColorsPainterFieldData.cxx -1
  )
vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
  TestScalarsToColorsPainterFieldDataTuple,TestScalarsToColorsPainterFieldData.cxx 50
  )
vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
  TestBlurAndSobelPasses.cxx
  #TestGaussianBlurPass.cxx # Missing symbol checking for Mesa bug
  TestLightActor.cxx
  TestSobelGradientMagnitudePass.cxx)
if(NOT APPLE)
  vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
    TestShadowMapPass.cxx)
endif()

if(WIN32 AND NOT VTK_USE_X)
  vtk_add_test_cxx(${vtk-module}CxxTests render_window_tests
    TestWin32OpenGLRenderWindow.cxx NO_VALID)
endif()

include_directories(${OPENGL_INCLUDE_DIR})

vtk_add_test_cxx(${vtk-module}CxxTests ogl_tests
  TestScenePicker.cxx,NO_VALID
  TestMultiblockDisplayProperties.cxx
  TestMultiTexturing.cxx
  TestMultiTexturingTransform.cxx)

set(VTK_USE_GLSL_SHADERS FALSE)
if(VTK_USE_GLSL_SHADERS)
  # Tests testing GLSL Shaders.
  vtk_add_test_cxx(${vtk-module}CxxTests glsl_tests
    TestGenericVertexAttributesGLSLCxx.cxx
    TestGenericVertexAttributesGLSLAlphaBlending.cxx
    TestGenericVertexAttributesGLSLDepthPeelingPass.cxx
    )
  list(APPEND ogl_tests
    ${glsl_tests})
endif()

set(all_tests
  ${render_window_tests}
  ${ogl_tests})
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests)

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
