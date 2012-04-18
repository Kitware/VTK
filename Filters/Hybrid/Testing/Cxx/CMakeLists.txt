Set(MyTests
  # TestBSplineWarp.cxx           # Fixme after vtkImageViewer deps
  TestPolyDataSilhouette.cxx
  TestTemporalCacheSimple.cxx
  TestTemporalCacheTemporal.cxx
  TestTemporalFractal.cxx
)

include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})

list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# eventually we want to move this in the loop, but we need this
# special case because the base line image is in Baseline/Parallel
set(temporalfractal-test TestTemporalFractal.cxx)
get_filename_component(TName ${temporalfractal-test} NAME_WE)

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/${TName}.png)
endif()
list(REMOVE_ITEM TestsToRun ${temporalfractal-test})

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Hybrid/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
