include(vtkGhostscript)

# Only run GL2PS tests if GhostScript is available
if(VTK_GHOSTSCRIPT_EXECUTABLE AND (VTK_RENDERING_BACKEND STREQUAL "OpenGL"))
  set(GL2PSTests
    TestContextGL2PS.cxx
    TestGL2PSExporterMultipleRenderers.cxx
    TestGL2PSExporterRaster.cxx
    TestGL2PSExporterRasterExclusion.cxx
    TestGL2PSExporterVector.cxx
    TestGL2PSExporterVolumeRaster.cxx
    TestGL2PSTextActor.cxx
    TestGL2PSTextActor3D.cxx
    TestGL2PSLabeledDataMapper.cxx
    TestLinePlotGL2PS.cxx
    TestStackedPlotGL2PS.cxx
  )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  X3DTest.cxx,NO_DATA,NO_VALID
  ${GL2PSTests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )

# The GL2PS tests produce postscript output, which cannot be compared using
# the built-in image regression framework. The tests added above will create the
# postscript, while the following tests will convert the postscript to a PNG and
# run the PNGCompare test target to do an image diff on the output against a
# baseline.
if(GL2PSTests)
  set(TestGL2PSExporterVolumeRasterError 50)
  foreach(test ${GL2PSTests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)
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
        LABELS "${${vtk-module}_TEST_LABELS}"
    )

    # Convert ps to png
    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()
    # Image diff rasterized png with baseline
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND vtkRenderingGL2PSCxxTests PNGCompare
        -D ${VTK_TEST_DATA_DIR}
        -T ${VTK_TEST_OUTPUT_DIR}
        -E ${_error_threshold}
        -V DATA{../Data/Baseline/${TName}-rasterRef.png,:}
        --test-file ${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-VerifyRasterizedPNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        LABELS "${${vtk-module}_TEST_LABELS}"
    )
  endforeach()
endif()
