vtk_module_test_data(
  Data/OpenFOAM/cavity/,REGEX:.*
  Data/OpenFOAM/cavity/0/,REGEX:.*
  Data/OpenFOAM/cavity/0.5/,REGEX:.*
  Data/OpenFOAM/cavity/0.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/1/,REGEX:.*
  Data/OpenFOAM/cavity/1/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/1.5/,REGEX:.*
  Data/OpenFOAM/cavity/1.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/2.5/,REGEX:.*
  Data/OpenFOAM/cavity/2.5/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/2/,REGEX:.*
  Data/OpenFOAM/cavity/2/uniform/,REGEX:.*
  Data/OpenFOAM/cavity/constant/,REGEX:.*
  Data/OpenFOAM/cavity/constant/polyMesh/,REGEX:.*
  Data/OpenFOAM/cavity/system/,REGEX:.*
  Data/bigendian.xyz)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/beach.tif
    Data/combq.bin
    Data/combxyz.bin
    Data/multicomb_0.vts)

  add_subdirectory(Python)
endif ()
