find_package(OSPRay REQUIRED)
include_directories(${OSPRAY_INCLUDE_DIRS})

vtk_add_test_cxx(vtkRenderingOSPRayCxxTests tests
  # Surface rendering tests
  TestOSPRayAmbient.cxx
  TestOSPRayAMRVolumeRenderer.cxx
  TestOSPRayCompositePolyDataMapper2.cxx
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
  TestSmartVolumeMapper.cxx
  # Path tracer tests
  TestCategoricalMaterials.cxx
  TestCategoricalMultiBlock.cxx
  TestOSPRayRendererType.cxx
  TestPathTracerBackground.cxx
  TestPathTracerLuminous.cxx
  TestPathTracerMaterialLibrary.cxx,NO_VALID
  TestPathTracerMaterials.cxx
  TestPathTracerShadows.cxx
  )
vtk_test_cxx_executable(vtkRenderingOSPRayCxxTests tests RENDERING_FACTORY
  vtkOSPRayTestInteractor.cxx
  )


# Do we have MPI and parallel support
if (${Module_vtkRenderingParallel} AND ${Module_vtkParallelMPI})
  vtk_module_dep_includes(vtkRenderingParallel)
  vtk_module_dep_includes(vtkParallelMPI)
  include_directories(
    ${vtkRenderingParallel_INCLUDE_DIRS}
    ${vtkRenderingParallel_DEPENDS_INCLUDE_DIRS}
    ${vtkParallelMPI_INCLUDE_DIRS}
    ${vtkParallelMPI_DEPENDS_INCLUDE_DIRS}
    )

  include(vtkMPI)
  set(vtkRenderingOSPRayCxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkRenderingOSPRayCxx-MPI tests
    TESTING_DATA
    TestOSPRayComposite.cxx
    )
  vtk_test_mpi_executable(vtkRenderingOSPRayCxx-MPI tests RENDERING_FACTORY
    vtkOSPRayTestInteractor.cxx
    )

  target_link_libraries(vtkRenderingOSPRayCxx-MPI PRIVATE
    ${vtkRenderingParallel_LIBRARIES}
    ${vtkParallelMPI_LIBRARIES}
    )
endif()
