set(MyTests
  HomogeneousRayIntegration.cxx
  LinearRayIntegration.cxx
  PartialPreIntegration.cxx
  PreIntegrationIncremental.cxx
  PreIntegrationNonIncremental.cxx
  TestFixedPointRayCastLightComponents.cxx
  TestGPURayCastAdditive.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  TestGPURayCastCompositeShadeMask.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastFourComponentsComposite.cxx
  TestGPURayCastFourComponentsCompositeStreaming.cxx
  TestGPURayCastFourComponentsMIP.cxx
  TestGPURayCastFourComponentsMinIP.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestProjectedHexahedra.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  TestTM3DLightComponents.cxx
  ZsweepConcavities.cxx
)

# Tests with data
if(VTK_DATA_ROOT)
  set(MyTests
    ${MyTests}
    ProjectedTetrahedraZoomIn.cxx
    TestFinalColorWindowLevel.cxx
    TestGPURayCastNearestDataTypesMIP.cxx
    TestGPURayCastDataTypesMinIP.cxx
    TestHAVSVolumeMapper.cxx
    TestMinIntensityRendering.cxx
    TestProjectedTetrahedra.cxx
    TestProp3DFollower.cxx
    volProt.cxx
    )
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests}
  ExerciseUnstructuredGridRayCastMapper.cxx )

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/VolumeRendering/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

if (VTK_DATA_ROOT)
  set_tests_properties(${vtk-module}Cxx-TestProp3DFollower
    PROPERTIES TIMEOUT 100)
endif()

set_tests_properties(${vtk-module}Cxx-TestTM3DLightComponents
  PROPERTIES TIMEOUT 120)
