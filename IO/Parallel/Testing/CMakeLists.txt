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

  Data/OpenFOAM/mixerGgi/,REGEX:.*
  Data/OpenFOAM/mixerGgi/processor0/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/mixerGgi/processor1/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/mixerGgi/processor2/,RECURSE:,REGEX:.*$
  Data/OpenFOAM/mixerGgi/processor3/,RECURSE:,REGEX:.*$

  Data/bigendian.xyz)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/beach.tif
    Data/combq.bin
    Data/combxyz.bin
    Data/multicomb_0.vts
    Data/nek5000/eddy_uv/,REGEX:.*
    Data/EnSight/elementTypesTest.case
    Data/EnSight/elementTypesTest.geo)

  add_subdirectory(Python)
endif ()
