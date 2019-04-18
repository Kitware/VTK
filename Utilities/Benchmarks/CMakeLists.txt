set(classes
  vtkRenderTimings)

vtk_module_add_module(VTK::UtilitiesBenchmarks
  CLASSES ${classes})

# Add our test executables.
vtk_module_add_executable(TimingTests
  NO_INSTALL
  TimingTests.cxx)
target_link_libraries(TimingTests
  PRIVATE
    VTK::UtilitiesBenchmarks)

vtk_module_autoinit(
  TARGETS TimingTests
  MODULES VTK::UtilitiesBenchmarks)

vtk_module_add_executable(GLBenchmarking
  NO_INSTALL
  GLBenchmarking.cxx)
target_link_libraries(GLBenchmarking
  PRIVATE
    VTK::ChartsCore
    VTK::UtilitiesBenchmarks
    VTK::ViewsContext2D)

vtk_module_autoinit(
  TARGETS GLBenchmarking
  MODULES VTK::ChartsCore
          VTK::UtilitiesBenchmarks
          VTK::ViewsContext2D)
