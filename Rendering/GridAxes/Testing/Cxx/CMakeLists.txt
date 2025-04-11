vtk_add_test_cxx(vtkRenderingGridAxesCxxTests tests
  TestGridAxesActor3D.cxx
  TestGridAxesActor2D.cxx
  )

vtk_test_cxx_executable(vtkRenderingGridAxesCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
