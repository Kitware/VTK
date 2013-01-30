# Check that matplotlib is available:
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import matplotlib"
  RESULT_VARIABLE IMPORT_MATPLOTLIB_EXITCODE
  OUTPUT_VARIABLE IMPORT_MATPLOTLIB_OUTPUT
  ERROR_VARIABLE IMPORT_MATPLOTLIB_ERROR
  )
set(MATPLOTLIB_FOUND FALSE)
if(${IMPORT_MATPLOTLIB_EXITCODE} EQUAL 0)
  set(MATPLOTLIB_FOUND TRUE)
endif()

if(NOT MATPLOTLIB_FOUND)
  message("Matplotlib not found! MathText rendering will not be available until it is installed. Disabling tests.")
endif()

if(MATPLOTLIB_FOUND)
  if(VTK_GHOSTSCRIPT_EXECUTABLE AND VTK_DATA_ROOT)
    set(GL2PSTests
      TestGL2PSMathTextActor.cxx
      TestGL2PSMathTextActor3D.cxx
      TestGL2PSMathTextOutput.cxx
      TestGL2PSMathTextScaling.cxx
    )
  endif()

  if(VTK_DATA_ROOT)
    # add tests that require data
    set(MyTests
      TestContextMathTextImage.cxx
      TestMathTextActor.cxx
      TestMathTextActor3D.cxx
      TestRenderString.cxx
      TestStringToPath.cxx
      TestIndexedLookupScalarBar.cxx
      ${GL2PSTests}
      )
  endif()
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
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}.png
        -E ${_error_threshold})
  else()
    add_test(NAME MathText-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()

# The GL2PS tests produce postscript output, which cannot be compared using
# the built-in image regression framework. The tests added above will create the
# postscript, while the following tests will convert the postscript to a PNG and
# run the PNGCompare test target to do an image diff on the output against a
# baseline.
if(GL2PSTests)
  foreach(test ${GL2PSTests} ${MixedTests})
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
    add_test(NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND vtkRenderingGL2PSCxxTests PNGCompare
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Rendering/${TName}-rasterRef.png
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
