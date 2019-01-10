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
    add_test(NAME ${_vtk_build_test}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DSVGFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.svg"
        "-DPNGFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        "-DCONVERTER=${VTK_WKHTMLTOIMAGE_EXECUTABLE}"
        -DREMOVESVG=1
        -P "${VTK_SOURCE_DIR}/CMake/RasterizeSVG.cmake"
    )
    set_tests_properties("${_vtk_build_test}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${_vtk_build_test}Cxx-${TName}"
        REQUIRED_FILES "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.svg"
        LABELS "${vtkIOExport_TEST_LABELS}"
    )

    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()

    # Image diff rasterized png produced from SVG with baseline
    ExternalData_add_test(VTKData
      NAME ${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND "vtkRenderingGL2PSOpenGL2CxxTests" PNGCompare
        -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}"
        -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
        -E "${_error_threshold}"
        -V "DATA{../Data/Baseline/${TName}-rasterRef.png,:}"
        --test-file "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
    )
    set_tests_properties("${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPNG"
      PROPERTIES
        DEPENDS "${_vtk_build_test}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        LABELS "${_vtk_build_test_labels}"
        )
  endforeach()
endfunction()

set(vtkTestingSVG_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
