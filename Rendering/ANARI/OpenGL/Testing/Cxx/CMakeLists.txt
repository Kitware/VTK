set(tests_with_baseline
  TestAnariLayers
  TestAnariPass
  TestAnariScalarBar
  TestAnariSkybox
  TestAnariSmartVolumeMapper
  TestAnariStereo
  TestAnariWindowResize
)

# Switching ANARI libraries only has to avoid crashing, so there is no baseline.
set(tests_without_baseline
  TestAnariLibrarySwitch
)

foreach(name IN LISTS tests_with_baseline)
  set(test_file_name "${name}.cxx")

  vtk_add_test_cxx(vtkRenderingAnariOpenGLCxxTests tests
    ${test_file_name},NO_SERDES)

  set_tests_properties(VTK::RenderingAnariOpenGLCxx-${name}
    PROPERTIES
    ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=true;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID"
    TIMEOUT 300
  )
endforeach()

foreach(name IN LISTS tests_without_baseline)
  set(test_file_name "${name}.cxx")

  vtk_add_test_cxx(vtkRenderingAnariOpenGLCxxTests tests
    ${test_file_name},NO_VALID,NO_SERDES)

  set_tests_properties(VTK::RenderingAnariOpenGLCxx-${name}
    PROPERTIES
    ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=true"
    TIMEOUT 300
  )
endforeach()

vtk_test_cxx_executable(vtkRenderingAnariOpenGLCxxTests tests
  RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkAnariOpenGLTestUtilities.cxx
)
