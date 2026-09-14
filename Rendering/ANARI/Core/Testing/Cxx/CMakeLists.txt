set(tests_with_baseline
  TestAnariActorVisibility
  TestAnariAmbient
  TestAnariCameraInside
  TestAnariCameraInsideSmallSpacing
  TestAnariCellData
  TestAnariCompositePolyDataMapper
  TestAnariCylinder
  TestAnariDynamicObject
  TestAnariGPUVolumeRayCastMapper
  TestAnariImplicits
  TestAnariLights
  TestAnariMaterialLibrary
  TestAnariMaterials
  TestAnariMultiBlock
  TestAnariMultiBlockPartialArrayFieldData
  TestAnariOrthographic
  TestAnariPBRMapping
  TestAnariPBRMaterials
  TestAnariPBRMaterialsCoat
  TestAnariPerspectiveParallel
  TestAnariPolyDataTexture
  TestAnariPositionalLights
  TestAnariRendererType
  TestAnariRenderMesh
  TestAnariRenderWindowRGB
  TestAnariRenderWindowRGBA
  TestAnariSphere
  TestAnariTransfer2D
  TestAnariVolumeLightKit
  TestAnariVolumeWavelet
)

set(tests_without_baseline
  TestAnariRendererParameters
)

foreach(test_name IN LISTS tests_with_baseline)
  vtk_add_test_cxx(vtkRenderingAnariCoreCxxTests anari_tests_with_baseline
    ${test_name}.cxx,NO_SERDES)
endforeach()

foreach(test_name IN LISTS tests_without_baseline)
  vtk_add_test_cxx(vtkRenderingAnariCoreCxxTests anari_tests_without_baseline
    ${test_name}.cxx,NO_SERDES,NO_VALID)
endforeach()

foreach(test_name IN LISTS tests_with_baseline tests_without_baseline)
  set_tests_properties(VTK::RenderingAnariCoreCxx-${test_name}
    PROPERTIES
    ENVIRONMENT "VTK_FACTORY_PREFER=SupportRenderPass=false;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID"
    TIMEOUT 150)
endforeach()

set(all_tests
  ${anari_tests_with_baseline}
  ${anari_tests_without_baseline}
)
vtk_test_cxx_executable(
  vtkRenderingAnariCoreCxxTests
  all_tests
  RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkAnariTestUtilities.cxx
)
