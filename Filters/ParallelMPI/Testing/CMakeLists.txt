vtk_module_test_data(
  Data/voronoiMesh.vtu
  Data/HTG/shell_3d.htg
  Data/HTG/random_multi_block.vtm
  Data/HTG/random_multi_block/random_multi_block_0_0.htg
  Data/HTG/random_multi_block/random_multi_block_1_0.htg
  )

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
