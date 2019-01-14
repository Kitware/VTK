vtk_module_test_data(
  Data/bunny.ply
  Data/dragon.ply
  Data/ironProt.vtk
  Data/many_blocks/many_blocks.vtm
  Data/many_blocks/many_blocks/,REGEX:.*
  Data/ospray_mats.json
  Data/ospray_mats.mtl
  Data/vase_1comp.vti
  Data/wintersun.jpg)

add_subdirectory(Cxx)
