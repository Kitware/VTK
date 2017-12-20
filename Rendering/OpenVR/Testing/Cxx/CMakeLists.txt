vtk_add_test_cxx(vtkRenderingOpenVRCxxTests tests
  TestDragon.cxx,NO_VALID
#  Medical.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkRenderingOpenVRCxxTests tests RENDERING_FACTORY)
