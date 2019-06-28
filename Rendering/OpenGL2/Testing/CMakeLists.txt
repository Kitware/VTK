vtk_module_test_data(
  Data/GIS/raster.tif
  Data/autoshop.jpg
  Data/bunny.ply
  Data/clouds.jpeg
  Data/dragon.ply
  Data/normalMapping.png
  Data/skybox-nx.jpg
  Data/skybox-ny.jpg
  Data/skybox-nz.jpg
  Data/skybox-px.jpg
  Data/skybox-py.jpg
  Data/skybox-pz.jpg
  Data/skybox/,REGEX:.*\.jpg
  Data/,REGEX:vtk_.*\.png
  Data/wintersun.jpg)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
