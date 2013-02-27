# Only run GL2PS tests if GhostScript is available if VTK_DATA_ROOT is set
if(VTK_GHOSTSCRIPT_EXECUTABLE AND VTK_DATA_ROOT)
  set(GL2PSTests
    TestContextGL2PS.cxx
    TestGL2PSExporterMultipleRenderers.cxx
    TestGL2PSExporterRaster.cxx
    TestGL2PSExporterRasterExclusion.cxx
    TestGL2PSExporterVector.cxx
    TestGL2PSExporterVolumeRaster.cxx
    TestGL2PSTextActor3D.cxx
    TestStackedPlotGL2PS.cxx
  )
endif()

set(MyTests
  X3DTest.cxx
)

if (VTK_DATA_ROOT)
  set(MyTests
    ${MyTest}
    ${GL2PSTests}
  )
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

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

# The GL2PS tests produce postscript output, which cannot be compared using
# the built-in image regression framework. The tests added above will create the
# postscript, while the following tests will convert the postscript to a PNG and
# run the PNGCompare test target to do an image diff on the output against a
# baseline.
if(GL2PSTests)
  set(TestGL2PSExporterVolumeRasterError 50)
  foreach(test ${GL2PSTests})
    add_test(NAME ${vtk-module}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        -DPSFILE=${VTK_TEST_OUTPUT_DIR}/${TName}.ps
        -DPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png
        -DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}
        -DREMOVEPS=1
        -P ${CMAKE_CURRENT_SOURCE_DIR}/../../../../CMake/RasterizePostScript.cmake
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/${TName}.ps"
    )

    # Convert ps to png
    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()
    # Image diff rasterized png with baseline
    add_test(NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND vtkRenderingGL2PSCxxTests PNGCompare
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -E ${_error_threshold}
        -V Baseline/Hybrid/${TName}-rasterRef.png
        --test-file ${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-VerifyRasterizedPNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
    )
  endforeach()
endif()
