vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOSPRayCompositePolyDataMapper2.cxx,
  TestOSPRayDynamicObject.cxx,
  TestOSPRayDynamicScene.cxx,
  TestOSPRayImplicits.cxx,
  TestOSPRayLayers.cxx,
  TestOSPRayLights.cxx,
  TestOSPRayMultiBlock.cxx
  TestOSPRayMultiBlockPartialArrayFieldData.cxx,
  TestOSPRayPass.cxx,
  TestOSPRayRenderMesh.cxx,
  TestOSPRayScalarBar.cxx
  TestSmartVolumeMapper.cxx
  TestOSPRayTime.cxx,
  TestOSPRayWindow.cxx
  # Volume rendering tests
  TestOSPRayVolumeRenderer.cxx
  TestGPURayCastAdditive.cxx
  TestGPURayCastCameraInside.cxx
  TestGPURayCastCameraInsideSmallSpacing.cxx
  TestGPURayCastMapperBenchmark.cxx
  TestGPURayCastMapperSampleDistance.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestGPURayCastVolumeLightKit.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  TestGPUVolumeRayCastMapper.cxx
  TestGPURayCastVolumePolyData.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY
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
  set(${vtk-module}Cxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(${vtk-module}Cxx-MPI tests
    TESTING_DATA
    TestOSPRayComposite.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests RENDERING_FACTORY
    vtkOSPRayTestInteractor.cxx
    )

  target_link_libraries(${vtk-module}Cxx-MPI PRIVATE
    ${vtkRenderingParallel_LIBRARIES}
    ${vtkParallelMPI_LIBRARIES}
    )
endif()
