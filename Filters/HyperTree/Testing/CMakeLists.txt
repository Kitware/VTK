vtk_module_test_data(
  Data/AMR/htg3d.htg
  Data/HTG/binary_1D_X_311_mask.htg
  Data/HTG/binary_2D_XY_331_mask.htg
  Data/HTG/binary_3D_333_mask.htg
  Data/HTG/donut_XZ_shift_2d.htg
  Data/HTG/ghost.htg
  Data/HTG/htg_for_axis_aligned_cut.htg
  Data/HTG/shell_3d.htg
  Data/HTG/single_cell_3d.htg
  Data/HTG/three_cells_3d.htg
  Data/TestHyperTreeGridBinary3DContourImplicitInteger.vtp)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
