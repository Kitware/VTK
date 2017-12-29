vtk_module_test_data(
  Data/GreenCircle.png
  Data/SainteHelens.dem
  Data/SampleStructGrid.vtk
  Data/Tango/TangoIcons.png
  Data/alphachannel.png
  Data/beach.jpg
  Data/can.ex2
  Data/chombo3d/,REGEX:.*
  Data/combq.bin
  Data/combxyz.bin
  Data/earth.ppm
  Data/quadraticTetra01.vtu
  Data/textureRGBA.png)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/Viewpoint/cow.g
    Data/Viewpoint/cow.obj
    Data/beach.tif
    Data/blow.vtk
    Data/headsq/,REGEX:.*
    Data/masonry.bmp)

  add_subdirectory(Python)
endif ()
