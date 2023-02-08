set(datasets
  Data/CGNS/fluid.cgns.4.0
  Data/CGNS/fluid.cgns.4.1
  Data/CGNS/fluid.cgns.4.2
  Data/CGNS/fluid.cgns.4.3
  Data/CGNS/r24_node_tailor_test_output/sparc-volume.cgns.36.00
  Data/Exodus/Assembly-Example.g
  Data/Exodus/biplane_rms_pressure_bs.exo
  Data/Exodus/block_with_attributes.g
  Data/Exodus/can.e.4/can.e.4.0
  Data/Exodus/can.e.4/can.e.4.1
  Data/Exodus/can.e.4/can.e.4.2
  Data/Exodus/can.e.4/can.e.4.3
  Data/Exodus/ExRestarts/blow.e
  Data/Exodus/ExRestarts/blow.e-s.001
  Data/Exodus/ExRestarts/blow.ex-timeseries
  Data/Exodus/ghost.ex2
  Data/Exodus/hello_world_fix-d_frf.frq
  Data/Exodus/RubiksCubeWithRotations_gold.g
  Data/Exodus/SAND2020-4077_O-tri6sWFace2.exo
  Data/Exodus/test-nfaced.exo
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.0
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.1
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.2
  Data/FSIobject/FSIobject.inp.EXODUS_DATA.e-s.00001.4.3)

if (VTK_USE_LARGE_DATA)
  list(APPEND datasets
    Data/Exodus/SAND2017-5827O-FSM_Residual-bad-eigen.e
    Data/Exodus/SAND2017-5827O-FSM_Residual_good-eigen.e)
endif()
vtk_module_test_data(${datasets})

add_subdirectory(Cxx)
