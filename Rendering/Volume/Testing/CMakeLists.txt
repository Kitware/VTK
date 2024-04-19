vtk_module_test_data(
  Data/can.ex2
  Data/HeadMRVolume.mhd
  Data/MagnitudeImage_256x256x148
  Data/headsq/,REGEX:.*
  Data/headmr3blocks/headmr3blocks.vtm
  Data/headmr3blocks/headmr3blocks/,REGEX:.*
  Data/hncma-atlas.vti
  Data/ironProt.vtk
  Data/neghip.slc
  Data/sphere.slc
  Data/tooth.nhdr
  Data/tooth.raw
  Data/tooth_2dtransf.png
  Data/vase_1comp.vti
  Data/vase_4comp.vti)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/m4_TotalDensity.cube)

  add_subdirectory(Python)
endif ()
