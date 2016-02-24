# vtk_add_gl2ps_test_cxx(<test> [<another test> <yet another test> ...])
#
# Takes a list of test source files as arguments and adds additional tests
# to convert a postscript output into png (RasterizePNG) and validates against a
# baseline (VerifyRasterizedPNG).
#
# This function does not replace vtk_add_test_cxx, but supplements it -- this
# only creates the rasterize/verify tests, vtk_add_test_cxx is needed to create
# the test that generates the original postscript.
function(vtk_add_gl2ps_test_cxx)
  foreach(test ${ARGN})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert ps to png
    add_test(NAME ${vtk-module}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DPSFILE=${VTK_TEST_OUTPUT_DIR}/${TName}.ps"
        "-DPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        "-DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}"
        -DREMOVEPS=1
        -P "${vtkTestingGL2PS_SOURCE_DIR}/RasterizePostScript.cmake"
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/${TName}.ps"
        LABELS "${${vtk-module}_TEST_LABELS}"
    )

    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()

    # Unit test executable containing PNGCompare test:
    if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
      set(PNGCompareTest vtkRenderingGL2PSCxxTests)
    elseif(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
      set(PNGCompareTest vtkRenderingGL2PSOpenGL2CxxTests)
    endif()

    # Image diff rasterized png with baseline
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND "${PNGCompareTest}" PNGCompare
        -D "${VTK_TEST_DATA_DIR}"
        -T "${VTK_TEST_OUTPUT_DIR}"
        -E "${_error_threshold}"
        -V "DATA{../Data/Baseline/${TName}-rasterRef.png,:}"
        --test-file "${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-VerifyRasterizedPNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES
          "${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        LABELS "${${vtk-module}_TEST_LABELS}"
    )
  endforeach()
endfunction()

set(vtkTestingGL2PS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
