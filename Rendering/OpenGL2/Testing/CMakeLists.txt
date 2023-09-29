vtk_module_test_data(
  Data/GIS/raster.tif
  Data/autoshop.jpg
  Data/anisotropyTex.png
  Data/bunny.ply
  Data/dgHexahedra.dg
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
  Data/spiaggia_di_mondello_1k.hdr
  Data/,REGEX:vtk_.*\.png
  Data/wintersun.jpg)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
