include(vtkGhostscript)

# Check that matplotlib is available
include(FindPythonModules)
find_python_module(matplotlib MATPLOTLIB_FOUND)

if(NOT MATPLOTLIB_FOUND)
  message(STATUS "Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests.")
endif()

if(MATPLOTLIB_FOUND)
  # add tests that require data
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestContextMathTextImage.cxx
    TestMathTextActor.cxx
    TestMathTextActor3D.cxx
    TestRenderString.cxx
    TestStringToPath.cxx
    TestIndexedLookupScalarBar.cxx
    TestScalarBarCombinatorics.cxx
    )

  if(VTK_GHOSTSCRIPT_EXECUTABLE)
    vtk_add_test_cxx(${vtk-module}CxxTests GL2PSTests
      TestGL2PSMathTextActor.cxx
      TestGL2PSMathTextActor3D.cxx
      TestGL2PSMathTextOutput.cxx
      TestGL2PSMathTextScaling.cxx
    )
    list(APPEND tests
      ${GL2PSTests})
  endif()

  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY)
endif()

# The GL2PS tests produce postscript output, which cannot be compared using
# the built-in image regression framework. The tests added above will create the
# postscript, while the following tests will convert the postscript to a PNG and
# run the PNGCompare test target to do an image diff on the output against a
# baseline.
if(GL2PSTests)

  foreach(test ${GL2PSTests})
    # Convert ps to png
    get_filename_component(TName ${test} NAME_WE)
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
    # Image diff rasterized png with baseline
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND vtkRenderingGL2PSCxxTests PNGCompare
        -D ${VTK_TEST_DATA_DIR}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V DATA{../Data/Baseline/${TName}-rasterRef.png,:}
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
