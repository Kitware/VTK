# vtk_add_svg_test(<test> [<another test> <yet another test> ...])
#
# Takes a list of test source files as arguments and adds additional tests
# to convert an SVG output into png (RasterizePNG) and validates against a
# baseline (VerifyRasterizedPNG).
#
# This function does not replace vtk_add_test_cxx, but supplements it -- this
# only creates the rasterize/verify tests, vtk_add_test_cxx is needed to create
# the test that generates the original SVG output.
function(vtk_add_svg_test)
  set(tests ${ARGN})
  foreach(test ${tests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert svg to png
    add_test(NAME ${vtk-module}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DSVGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}.svg"
        "-DPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        "-DCONVERTER=${VTK_WKHTMLTOIMAGE_EXECUTABLE}"
        -DREMOVESVG=1
        -P "${vtkTestingSVG_SOURCE_DIR}/RasterizeSVG.cmake"
    )
    set_tests_properties("${vtk-module}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}"
        REQUIRED_FILES "${VTK_TEST_OUTPUT_DIR}/${TName}.svg"
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

    # Image diff rasterized png produced from SVG with baseline
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
        REQUIRED_FILES "${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        LABELS "${${vtk-module}_TEST_LABELS}"
        )
  endforeach()
endfunction()

set(vtkTestingSVG_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
