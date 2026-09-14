set(test_names
  TestAnariLayers
  TestAnariPass
  TestAnariScalarBar
  TestAnariSmartVolumeMapper
  TestAnariStereo
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

vtk_test_cxx_executable(vtkRenderingAnariOpenGLCxxTests tests
  RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkAnariOpenGLTestUtilities.cxx
)
