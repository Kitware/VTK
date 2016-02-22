vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOsprayRenderMesh.cxx,
  TestOsprayPass.cxx,
  TestOsprayDynamicScene.cxx,
  TestOsprayDynamicObject.cxx,
  TestOsprayLayers.cxx,
  TestOsprayMultiBlock.cxx
  TestOsprayTime.cxx
  #TODO: interaction, pure offscreen, volumes
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY
  vtkOsprayTestInteractor.cxx
  )

if(VTK_MPI_MAX_NUMPROCS GREATER 1)
  include(vtkMPI)
  set(${vtk-module}Cxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(${vtk-module}Cxx-MPI tests
    TESTING_DATA
    TestOsprayComposite.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests RENDERING_FACTORY
    vtkOsprayTestInteractor.cxx
  )
endif()
