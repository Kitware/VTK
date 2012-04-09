set(MyTests
  # TestBSplineWarp.cxx           # Fixme after vtkImageViewer deps
  TestPolyDataSilhouette.cxx
)

# Use the testing object factory, to reduce boilerplate code in tests.
include("${VTK_SOURCE_DIR}/Testing/Rendering/vtkTestingObjectFactory.cmake")

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-PolyDataSilhouette
    COMMAND ${vtk-module}CxxTests TestPolyDataSilhouette
      ${VTK_DATA_ROOT}/Data/cow.vtp
      -D ${VTK_DATA_ROOT}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      -V Baseline/Hybrid/TestPolyDataSilhouette.png)
endif()
