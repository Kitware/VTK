vtk_add_test_cxx(vtkRenderingOculusCxxTests tests
  TestDragon.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkRenderingOculusCxxTests tests RENDERING_FACTORY)
