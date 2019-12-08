vtk_module_test_data(
  Data/Quadratic/CylinderQuadratic.vtk
  Data/Tango/TangoIcons.png
  Data/UCD2D/UCD_00005.inp
  Data/fullhead15.png
  Data/hexa.vtk
  Data/mine_with_dates.vtp
  Data/quadraticTetra01.vtu
  Data/waveletMaterial.vti)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/42400-IDGH.stl
    Data/EnSight/,REGEX:elements\..*
    Data/RectGrid2.vtk
    Data/RedCircle.png
    Data/SampleStructGrid.vtk
    Data/camscene.png
    Data/can.ex2
    Data/combq.bin
    Data/combxyz.bin
    Data/earth.ppm
    Data/headsq/,REGEX:.*
    Data/ironProt.vtk
    Data/office.binary.vtk
    Data/uGridEx.vtk)

  add_subdirectory(Python)
endif ()
