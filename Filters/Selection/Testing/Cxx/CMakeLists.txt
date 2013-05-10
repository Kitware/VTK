vtk_add_test_cxx(Graphics
  TestLinearSelector2D.cxx
  )

# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(Graphics
    TestCellDistanceSelector2D.cxx
    TestCellDistanceSelector3D.cxx
    TestLinearSelector3D.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
