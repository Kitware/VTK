# Find the GhostScript executable for GL2PS tests.
find_program(VTK_GHOSTSCRIPT_EXECUTABLE gs gswin32c gsos2)
mark_as_advanced(VTK_GHOSTSCRIPT_EXECUTABLE)

# vtk_add_gl2ps_test_cxx([pdf] <test> [<another test> <yet another test> ...])
#
# Takes a list of test source files as arguments and adds additional tests
# to convert a postscript output into png (RasterizePNG) and validates against a
# baseline (VerifyRasterizedPNG).
#
# If the first parameter is pdf, besides validating postscript files it validates pdf
# files using the same procedure. RasterizePNG will generate two pngs, one for the
# postscript file and one for the PDF file. We'll have two validation tests: VerifyRasterizedPNG
# and VerifyRasterizedPDFPNG
#
# This function does not replace vtk_add_test_cxx, but supplements it -- this
# only creates the rasterize/verify tests, vtk_add_test_cxx is needed to create
# the test that generates the original postscript.
function(vtk_add_gl2ps_test_cxx)
  set(tests ${ARGN})
  if(${ARGC} GREATER 0 AND "${ARGV0}" STREQUAL "pdf")
    set (RASTERIZE_PDF TRUE)
    list(REMOVE_AT tests 0)
  else()
    set (RASTERIZE_PDF FALSE)
  endif()
  foreach(test ${tests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert ps to png
    add_test(NAME ${_vtk_build_test}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DPSFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.ps"
        "-DPNGFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        "-DPDFPNGFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster-pdf.png"
        "-DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}"
        -DREMOVEPS=1
        -DRASTERIZE_PDF=${RASTERIZE_PDF}
        -P "${vtkTestingGL2PS_SOURCE_DIR}/RasterizePostScript.cmake"
    )
    set_tests_properties("${_vtk_build_test}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${_vtk_build_test}Cxx-${TName}"
        REQUIRED_FILES
          "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.ps"
        LABELS "${_vtk_build_test_labels}"
    )

    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()

    # Image diff rasterized png produced from a PS with baseline
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
        REQUIRED_FILES
          "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        LABELS "${_vtk_build_test_labels}"
        )
    if(${RASTERIZE_PDF})
      ExternalData_add_test(VTKData
        NAME ${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPDFPNG
        COMMAND "vtkRenderingGL2PSOpenGL2CxxTests" PNGCompare
        -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}"
        -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
        -E "${_error_threshold}"
        -V "DATA{../Data/Baseline/${TName}-rasterRef.png,:}"
        --test-file "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster-pdf.png"
        )
      set_tests_properties("${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPDFPNG"
        PROPERTIES
        DEPENDS "${_vtk_build_test}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES
        "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster-pdf.png"
        LABELS "${_vtk_build_test_labels}"
        )
    endif()
  endforeach()
endfunction()

set(vtkTestingGL2PS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
