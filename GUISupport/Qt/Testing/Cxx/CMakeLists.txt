set(MyTests
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx ${MyTests}
  EXTRA_INCLUDE vtkTestDriver.h)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

vtk_module_test_executable(${vtk-module}CxxTests ${Tests} QTestApp.cxx)

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/QVTK/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
