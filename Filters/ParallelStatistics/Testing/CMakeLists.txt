vtk_module_test_data(
  # Can simulation split across 4 ranks, plus expected statistical model tables:
  Data/Exodus/can.e.4/can.e.4.0
  Data/Exodus/can.e.4/can.e.4.1
  Data/Exodus/can.e.4/can.e.4.2
  Data/Exodus/can.e.4/can.e.4.3
  Data/Exodus/can.e.4/statistics/TestModel_B.vtpc
  Data/Exodus/can.e.4/statistics/TestModel_B/TestModel_B_0_0.vtstat
  Data/Exodus/can.e.4/statistics/TestModel_C.vtpc
  Data/Exodus/can.e.4/statistics/TestModel_C/TestModel_C_0_0.vtstat
  Data/Exodus/can.e.4/statistics/TestModel_C/TestModel_C_1_0.vtstat
  Data/Exodus/can.e.4/statistics/TestModel_D.vtpc
  Data/Exodus/can.e.4/statistics/TestModel_D/TestModel_D_0_0.vtstat
  Data/Exodus/can.e.4/statistics/TestModel_D/TestModel_D_1_0.vtstat

  # Simple quads split across 2 ranks, plus expected statistical model tables:
  Data/simple-stat.vtpc
  Data/simple-stat/simple-stat_0_1.vtu
  Data/simple-stat/simple-stat_1_0.vtu
  Data/simple-stat/simple-stat_1_1.vtu
  Data/simple-stat/simple-stat_0_0.vtu
  Data/simple-stat/TestModelSimple_B.vtpc
  Data/simple-stat/TestModelSimple_C.vtpc
  Data/simple-stat/TestModelSimple_B/TestModelSimple_B_0_0.vtstat
  Data/simple-stat/TestModelSimple_C/TestModelSimple_C_0_0.vtstat
  Data/simple-stat/TestModelSimple_C/TestModelSimple_C_1_0.vtstat
)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
