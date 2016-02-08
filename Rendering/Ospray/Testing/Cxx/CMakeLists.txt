vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOsprayDemo.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOspraySceneGraph.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOsprayRenderMesh.cxx,NO_DATA,NO_VALID
  TestOsprayPass.cxx,NO_DATA,NO_VALID
  TestOsprayDynamicScene.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOsprayDynamicObject.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOsprayLayers.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOsprayMultiBlock.cxx,NO_VALID
  TestOsprayTime.cxx,NO_VALID
  #TODO: interaction, pure offscreen, volumes, textures
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  vtkOsprayTestInteractor.cxx
  )

if(VTK_MPI_MAX_NUMPROCS GREATER 1)
  include(vtkMPI)
  set(${vtk-module}Cxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(${vtk-module}Cxx-MPI tests
    TESTING_DATA
    TestOsprayComposite.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests
    vtkOsprayTestInteractor.cxx
  )
endif()
