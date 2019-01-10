# Find the GhostScript executable for GL2PS tests.
find_program(VTK_GHOSTSCRIPT_EXECUTABLE gs gswin32c gsos2)
mark_as_advanced(VTK_GHOSTSCRIPT_EXECUTABLE)

# Like the above, but only tests PDF (instead of always PS and maybe PDF).
function(vtk_add_pdf_test_cxx)
  set(tests ${ARGN})
  foreach(test ${tests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert pdf to png
    add_test(NAME ${_vtk_build_test}Cxx-${TName}-RasterizePDFToPNG
      COMMAND ${CMAKE_COMMAND}
      "-DPDFFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.pdf"
      "-DPDFPNGFILE=${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-rasterPDF.png"
      "-DGS_EXECUTABLE=${VTK_GHOSTSCRIPT_EXECUTABLE}"
      -DREMOVEPDF=1
      -DRASTERIZE_PDF=1
      -P "${vtkTestingPDF_SOURCE_DIR}/RasterizePostScript.cmake"
      )
    set_tests_properties("${_vtk_build_test}Cxx-${TName}-RasterizePDFToPNG"
      PROPERTIES
      DEPENDS "${_vtk_build_test}Cxx-${TName}"
      REQUIRED_FILES
      "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.pdf"
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
      NAME ${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPDFPNG
      COMMAND "vtkRenderingGL2PSOpenGL2CxxTests" PNGCompare
      -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}"
      -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}"
      -E "${_error_threshold}"
      -V "DATA{../Data/Baseline/${TName}-rasterPDFRef.png,:}"
      --test-file "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-rasterPDF.png"
      )
    set_tests_properties("${_vtk_build_test}Cxx-${TName}-VerifyRasterizedPDFPNG"
      PROPERTIES
      DEPENDS "${_vtk_build_test}Cxx-${TName}-RasterizePDFToPNG"
      REQUIRED_FILES
      "${_vtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-rasterPDF.png"
      LABELS "${_vtk_build_test_labels}"
      )
  endforeach()
endfunction()

set(vtkTestingPDF_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
