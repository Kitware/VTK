
find_package( OptiX REQUIRED )
find_package( CUDA 4.0 REQUIRED )

include_directories(
    "${OptiX_INCLUDE}"
    "${CUDA_INCLUDE_DIRS}"
    )

vtk_add_test_cxx(vtkRenderingOptiXCxxTests tests
  TestOptiXCompositePolyDataMapper2.cxx,
  TestOptiXDynamicObject.cxx,
  TestOptiXDynamicScene.cxx,
  TestOptiXImplicits.cxx,
  TestOptiXLayers.cxx,
  TestOptiXLights.cxx,
  TestOptiXMultiBlock.cxx
  TestOptiXMultiBlockPartialArrayFieldData.cxx,
  TestOptiXPass.cxx,
  TestOptiXRenderMesh.cxx,
  TestOptiXScalarBar.cxx
  TestOptiXTime.cxx,
  TestOptiXWindow.cxx
  )
vtk_test_cxx_executable(vtkRenderingOptiXCxxTests tests RENDERING_FACTORY
  vtkOptiXTestInteractor.cxx
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
  set(vtkRenderingOptiXCxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkRenderingOptiXCxx-MPI tests
    TESTING_DATA
    TestOptiXComposite.cxx
    )
  vtk_test_mpi_executable(vtkRenderingOptiXCxx-MPI tests RENDERING_FACTORY
    vtkOptiXTestInteractor.cxx
    )

  target_link_libraries(vtkRenderingOptiXCxx-MPI PRIVATE
    ${vtkRenderingParallel_LIBRARIES}
    ${vtkParallelMPI_LIBRARIES}
    )
endif()
