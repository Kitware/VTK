if (VTK_ENABLE_OSPRAY)
  vtk_add_test_cxx(vtkRenderingRayTracingCxxTests tests
    # Surface rendering tests
    TestOSPRayAmbient.cxx
#   TestOSPRayAMRVolumeRenderer.cxx #19221
    TestOSPRayCompositePolyDataMapper2.cxx
    TestOSPRayDepthOfField.cxx
    TestOSPRayDynamicObject.cxx
    TestOSPRayDynamicScene.cxx
    TestOSPRayImplicits.cxx
    TestOSPRayIsosurface.cxx
    TestOSPRayLayers.cxx
    TestOSPRayLights.cxx
    TestOSPRayMultiBlock.cxx
#   TestOSPRayMultiBlockPartialArrayFieldData.cxx #19221
    TestOSPRayOrthographic.cxx
    TestOSPRayPass.cxx
    TestOSPRayPointGaussianMapper.cxx
    TestOSPRayRenderMesh.cxx
    TestOSPRayRotatedNormals.cxx
    TestOSPRayScalarBar.cxx
    TestOSPRayTime.cxx
    TestOSPRayUnstructuredVolumeMapper.cxx
    TestOSPRayUnstructuredVolumeMapperMixedCells.cxx
    TestOSPRayWindow.cxx
    TestOSPRayStereo.cxx
    TestRayTracedMolecules.cxx
    # Volume rendering tests
    TestGPURayCastCameraInside.cxx
    TestGPURayCastCellData.cxx
    TestGPURayCastCameraInsideSmallSpacing.cxx
    TestGPURayCastMapperBenchmark.cxx
    TestGPURayCastMapperSampleDistance.cxx
    TestGPURayCastPerspectiveParallel.cxx
    #TestGPURayCastPositionalLights.cxx
    TestGPURayCastVolumeLightKit.cxx
    TestGPURayCastVolumePolyData.cxx
    TestGPURayCastVolumeScale.cxx
    TestGPURayCastVolumeUpdate.cxx
    TestGPUVolumeRayCastMapper.cxx
#   TestOSPRayVolumeRenderer.cxx #19221
    TestOSPRayVolumeRendererCrop.cxx
#   TestSmartVolumeMapper.cxx #19221
    # Path tracer tests
    TestCategoricalMaterials.cxx
    TestCategoricalMultiBlock.cxx
    TestPathTracerBackground.cxx
    TestPathTracerGLTFImporter.cxx DATA{../Data/WaterBottle.glb}
    TestPathTracerLuminous.cxx
    TestPathTracerMaterialLibrary.cxx,NO_VALID
    TestPathTracerMaterials.cxx
    TestPathTracerShadows.cxx
    # Miscellaneous tests
    TestOSPRayCache.cxx
    TestOSPRayTiling.cxx
    # Widgets tests
    TestOSPRayBoxWidget2.cxx
    TestOSPRayImplicitPlaneWidget2.cxx
    TestOSPRayPointCloudWidget.cxx
    TestOSPRayPointHandleRepresentation3D.cxx
    TestOSPRayRepresentationVisibility.cxx
    TestOSPRaySplineWidget2.cxx
  )
endif()

if (VTK_ENABLE_VISRTX)
  # OSPRay tests that are rerun with the OptiX backend
  set(optix_tests
    TestOSPRayAmbient
    TestOSPRayCompositePolyDataMapper2
    TestOSPRayDepthOfField
    TestOSPRayDynamicObject
    TestOSPRayDynamicScene
    TestOSPRayImplicits
    TestOSPRayLayers
    TestOSPRayLights
    TestOSPRayMultiBlock
#   TestOSPRayMultiBlockPartialArrayFieldData #19221
    TestOSPRayOrthographic
    TestOSPRayPass
    TestOSPRayRenderMesh
    TestOSPRayRotatedNormals
    TestOSPRayScalarBar
    TestOSPRayTime
    TestOSPRayWindow
    TestOSPRayStereo
    TestCategoricalMaterials
    # TestCategoricalMultiBlock #Disabled because of unknown instability after OSPRay 2 upgrade. Could not reproduce on NV side.
    TestPathTracerBackground
    TestPathTracerLuminous
    #TestPathTracerMaterials #Disabled because of unknown instability after OSPRay 2 upgrade. Could not reproduce on NV side.
    TestPathTracerShadows
    TestOSPRayCache
    #TestOSPRayTiling #Disabled because of unknown instability after OSPRay 2 upgrade. Could not reproduce on NV side.
    )

  set(testname_suffix "OptiX")
  set(optix_test_sources)
  foreach (tname IN LISTS optix_tests)
    list(APPEND optix_test_sources ${tname}${testname_suffix},${tname}.cxx)
  endforeach ()

  vtk_add_test_cxx(vtkRenderingRayTracingCxxTests tests_optix
    "--OptiX"
    ${optix_test_sources})
endif ()

if (VTK_ENABLE_OSPRAY AND VTK_ENABLE_VISRTX)
  # Tests combining both OSPRay and OptiX backends
  vtk_add_test_cxx(vtkRenderingRayTracingCxxTests tests_combined
    TestRendererType.cxx
    TestMultipleBackends.cxx
    )
endif ()

set(all_tests ${tests} ${tests_optix} ${tests_combined})
vtk_test_cxx_executable(vtkRenderingRayTracingCxxTests all_tests RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkOSPRayTestInteractor.cxx
  )


# Do we have MPI and parallel support
if (VTK_ENABLE_OSPRAY AND TARGET VTK::RenderingParallel AND TARGET VTK::ParallelMPI)
  set(vtkRenderingRayTracingCxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkRenderingRayTracingCxx-MPI tests
    TESTING_DATA
    TestOSPRayComposite.cxx
    )
  vtk_test_cxx_executable(vtkRenderingRayTracingCxx-MPI tests RENDERING_FACTORY
    vtkOSPRayTestInteractor.cxx
    )
endif ()
