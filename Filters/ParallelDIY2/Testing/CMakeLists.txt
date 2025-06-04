vtk_module_test_data(
  Data/cow.vtp
  Data/disk_out_ref.ex2
  Data/linhex.vtu
  Data/multiblock_overlapping_tetras/multiblock_overlapping_tetras_0_0.vtu
  Data/multiblock_overlapping_tetras/multiblock_overlapping_tetras_1_0.vtu
  Data/multiblock_overlapping_tetras.vtm
  Data/multicomb_0.vts
  Data/multicomb_1.vts
  Data/multicomb_2.vts
  Data/tet_hex.vtu)
if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
