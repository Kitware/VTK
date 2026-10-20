set(test_names
  TestAnariLayers
  TestAnariPass
  TestAnariScalarBar
  TestAnariSkybox
  TestAnariSmartVolumeMapper
  TestAnariStereo
  TestAnariWindowResize
)

foreach(name IN LISTS test_names)
  set(test_file_name "${name}.cxx")

  vtk_add_test_cxx(vtkRenderingAnariOpenGLCxxTests tests
    ${test_file_name},NO_SERDES)

  set_tests_properties(VTK::RenderingAnariOpenGLCxx-${name}
    PROPERTIES
    ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=true;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID"
    TIMEOUT 300
  )
endforeach()

# Switching ANARI libraries only has to avoid crashing, so there is no baseline.
vtk_add_test_cxx(vtkRenderingAnariOpenGLCxxTests tests
  TestAnariLibrarySwitch.cxx,NO_VALID,NO_SERDES)

set_tests_properties(VTK::RenderingAnariOpenGLCxx-TestAnariLibrarySwitch
  PROPERTIES
  ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=true"
  TIMEOUT 300
)

vtk_test_cxx_executable(vtkRenderingAnariOpenGLCxxTests tests
  RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkAnariOpenGLTestUtilities.cxx
)
