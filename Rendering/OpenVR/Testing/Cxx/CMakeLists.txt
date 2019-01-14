vtk_add_test_cxx(vtkRenderingOpenVRCxxTests tests
  TestAvatar.cxx
#  Medical.cxx,NO_VALID
  )

if (WIN32)
  vtk_add_test_cxx(vtkRenderingOpenVRCxxTests tests
    TestDragon.cxx,NO_VALID
    )
endif ()

vtk_test_cxx_executable(vtkRenderingOpenVRCxxTests tests RENDERING_FACTORY)
