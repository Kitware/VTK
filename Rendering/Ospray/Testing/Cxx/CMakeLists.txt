vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOsprayCompositePolyDataMapper2.cxx,
  TestOsprayDynamicObject.cxx,
  TestOsprayDynamicScene.cxx,
  TestOsprayImplicits.cxx,
  TestOsprayLayers.cxx,
  TestOsprayLights.cxx,
  TestOsprayMultiBlock.cxx
  TestOsprayMultiBlockPartialArrayFieldData.cxx,
  TestOsprayPass.cxx,
  TestOsprayRenderMesh.cxx,
  TestOsprayScalarBar.cxx
  TestOsprayTime.cxx,
  TestOsprayWindow.cxx
  #TODO: interaction, pure offscreen, volumes
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY
  vtkOsprayTestInteractor.cxx
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
    TestOsprayComposite.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests RENDERING_FACTORY
    vtkOsprayTestInteractor.cxx
    )

  target_link_libraries(${vtk-module}Cxx-MPI PRIVATE
    ${vtkRenderingParallel_LIBRARIES}
    ${vtkParallelMPI_LIBRARIES}
    )
endif()
