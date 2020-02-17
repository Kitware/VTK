if (VTK_ENABLE_OSPRAY)
  vtk_add_test_cxx(vtkRenderingRayTracingCxxTests tests
    # Surface rendering tests
    TestOSPRayAmbient.cxx
    TestOSPRayAMRVolumeRenderer.cxx
    TestOSPRayCompositePolyDataMapper2.cxx
    TestOSPRayDepthOfField.cxx
    TestOSPRayDynamicObject.cxx
    TestOSPRayDynamicScene.cxx
    TestOSPRayImplicits.cxx
    TestOSPRayIsosurface.cxx
    TestOSPRayLayers.cxx
    TestOSPRayLights.cxx
    TestOSPRayMultiBlock.cxx
    TestOSPRayMultiBlockPartialArrayFieldData.cxx
    TestOSPRayOrthographic.cxx
    TestOSPRayPass.cxx
    TestOSPRayRenderMesh.cxx
    TestOSPRayRotatedNormals.cxx
    TestOSPRayScalarBar.cxx
    TestOSPRayTetrahedraMapper.cxx
    TestOSPRayTetrahedraMapperMixedCells.cxx
    TestOSPRayTime.cxx
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
    TestOSPRayVolumeRenderer.cxx
    TestOSPRayVolumeRendererCrop.cxx
    TestSmartVolumeMapper.cxx
    # Path tracer tests
    TestCategoricalMaterials.cxx
    TestCategoricalMultiBlock.cxx
    TestPathTracerBackground.cxx
    TestPathTracerLuminous.cxx
    TestPathTracerMaterialLibrary.cxx,NO_VALID
    TestPathTracerMaterials.cxx
    TestPathTracerShadows.cxx
    # Miscellaneous tests
    TestOSPRayCache.cxx
    TestOSPRayTiling.cxx
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
    TestOSPRayMultiBlockPartialArrayFieldData
    TestOSPRayOrthographic
    TestOSPRayPass
    TestOSPRayRenderMesh
    TestOSPRayRotatedNormals
    TestOSPRayScalarBar
    TestOSPRayTime
    TestOSPRayWindow
    TestOSPRayStereo
    TestCategoricalMaterials
    TestCategoricalMultiBlock
    TestPathTracerBackground
    TestPathTracerLuminous
    TestPathTracerMaterials
    TestPathTracerShadows
    TestOSPRayCache
    TestOSPRayTiling
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
  vtkOSPRayTestInteractor.cxx
  )


# Do we have MPI and parallel support
if (VTK_ENABLE_OSPRAY AND TARGET vtkRenderingParallel AND TARGET vtkParallelMPI)
  set(vtkRenderingRayTracingCxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkRenderingRayTracingCxx-MPI tests
    TESTING_DATA
    TestOSPRayComposite.cxx
    )
  vtk_test_cxx_executable(vtkRenderingRayTracingCxx-MPI tests RENDERING_FACTORY
    vtkOSPRayTestInteractor.cxx
    )

  target_link_libraries(vtkRenderingRayTracingCxx-MPI
    PRIVATE
      vtkRenderingParallel
      vtkParallelMPI)
endif ()
