vtk_module_test_data(
  Data/bunny.ply
  Data/many_blocks/many_blocks.vtm
  Data/many_blocks/many_blocks/,REGEX:.*)

add_subdirectory(Cxx)
