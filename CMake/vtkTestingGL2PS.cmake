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
    add_test(NAME ${vtk-module}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DPSFILE=${VTK_TEST_OUTPUT_DIR}/${TName}.ps"
        "-DPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-raster.png"
        "-DPDFPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-raster-pdf.png"
        "-DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}"
        -DREMOVEPS=1
        -DRASTERIZE_PDF=${RASTERIZE_PDF}
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

    # Image diff rasterized png produced from a PS with baseline
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
    if(${RASTERIZE_PDF})
      ExternalData_add_test(VTKData
        NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPDFPNG
        COMMAND "${PNGCompareTest}" PNGCompare
        -D "${VTK_TEST_DATA_DIR}"
        -T "${VTK_TEST_OUTPUT_DIR}"
        -E "${_error_threshold}"
        -V "DATA{../Data/Baseline/${TName}-rasterRef.png,:}"
        --test-file "${VTK_TEST_OUTPUT_DIR}/${TName}-raster-pdf.png"
        )
      set_tests_properties("${vtk-module}Cxx-${TName}-VerifyRasterizedPDFPNG"
        PROPERTIES
        DEPENDS "${vtk-module}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES
        "${VTK_TEST_OUTPUT_DIR}/${TName}-raster-pdf.png"
        LABELS "${${vtk-module}_TEST_LABELS}"
        )
    endif()
  endforeach()
endfunction()

# Like the above, but only tests PDF (instead of always PS and maybe PDF).
function(vtk_add_pdf_test_cxx)
  set(tests ${ARGN})
  foreach(test ${tests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert pdf to png
    add_test(NAME ${vtk-module}Cxx-${TName}-RasterizePDFToPNG
      COMMAND ${CMAKE_COMMAND}
      "-DPDFFILE=${VTK_TEST_OUTPUT_DIR}/${TName}.pdf"
      "-DPDFPNGFILE=${VTK_TEST_OUTPUT_DIR}/${TName}-rasterPDF.png"
      "-DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}"
      -DREMOVEPDF=1
      -DRASTERIZE_PDF=1
      -P "${vtkTestingGL2PS_SOURCE_DIR}/RasterizePostScript.cmake"
      )
    set_tests_properties("${vtk-module}Cxx-${TName}-RasterizePDFToPNG"
      PROPERTIES
      DEPENDS "${vtk-module}Cxx-${TName}"
      REQUIRED_FILES
      "${VTK_TEST_OUTPUT_DIR}/${TName}.pdf"
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

    # Image diff rasterized png produced from a PS with baseline
    ExternalData_add_test(VTKData
      NAME ${vtk-module}Cxx-${TName}-VerifyRasterizedPDFPNG
      COMMAND "${PNGCompareTest}" PNGCompare
      -D "${VTK_TEST_DATA_DIR}"
      -T "${VTK_TEST_OUTPUT_DIR}"
      -E "${_error_threshold}"
      -V "DATA{../Data/Baseline/${TName}-rasterPDFRef.png,:}"
      --test-file "${VTK_TEST_OUTPUT_DIR}/${TName}-rasterPDF.png"
      )
    set_tests_properties("${vtk-module}Cxx-${TName}-VerifyRasterizedPDFPNG"
      PROPERTIES
      DEPENDS "${vtk-module}Cxx-${TName}-RasterizePDFToPNG"
      REQUIRED_FILES
      "${VTK_TEST_OUTPUT_DIR}/${TName}-rasterPDF.png"
      LABELS "${${vtk-module}_TEST_LABELS}"
      )
  endforeach()
endfunction()

set(vtkTestingGL2PS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
