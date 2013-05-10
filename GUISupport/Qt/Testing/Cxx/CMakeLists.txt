vtk_add_test_cxx(QVTK
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

vtk_test_cxx_executable(${vtk-module}CxxTests QTestApp.cxx)
