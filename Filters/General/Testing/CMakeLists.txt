vtk_module_test_data(
  Data/HTG/random_attributes.htg
  Data/HTG/random_partial_ZX.htg
  Data/HTG/shell_3d.htg
  Data/Quadratic/CylinderQuadratic.vtk
  Data/Tango/TangoIcons.png
  Data/TriangulatorBadData.vtk
  Data/UCD2D/UCD_00005.inp
  Data/2DScalar.vti
  Data/can.vtu
  Data/cow.vtp
  Data/distTest.vtm
  Data/explicitStructuredGrid.vtu
  Data/fullhead15.png
  Data/hexa.vtk
  Data/mb-of-mps.vtm
  Data/mb_single_piece_empty_data.vtm
  Data/mine_with_dates.vtp
  Data/quadraticTetra01.vtu
  Data/rectGrid.vtr
  Data/scalars.vti
  Data/sphereMirror.vtpc
  Data/sphereMirror_0_0.vtp
  Data/sphereMirror_1_0.vtp
  Data/structGrid.vts
  Data/waveletMaterial.vti)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

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
