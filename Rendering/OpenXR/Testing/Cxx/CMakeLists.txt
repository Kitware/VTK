vtk_add_test_cxx(vtkRenderingOpenXRCxxTests tests
  TestOpenXRInitialization.cxx
  )

# if (WIN32)
#   vtk_add_test_cxx(vtkRenderingOpenXRCxxTests tests
#     TestDragon.cxx,NO_VALID
#     )
# endif ()

# vtk_test_cxx_executable(vtkRenderingOpenXRCxxTests tests RENDERING_FACTORY)
vtk_test_cxx_executable(vtkRenderingOpenXRCxxTests tests)
