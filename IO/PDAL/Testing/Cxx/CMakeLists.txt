# TODO: Simplify this with some argument passing.
set(VTK_PDAL_READER_TESTS)
function(add_vtk_pdal_reader_test test_input elevation)
  get_filename_component(VTK_PDAL_READER_POSTFIX ${test_input} NAME_WE)
  string(REPLACE "-" "_" VTK_PDAL_READER_POSTFIX ${VTK_PDAL_READER_POSTFIX})
  set(VTK_PDAL_READER_TEST_INPUT "${test_input}")
  set(VTK_PDAL_READER_ELEVATION "${elevation}")
  set(test_source "TestPDALReader_${VTK_PDAL_READER_POSTFIX}.cxx")
  configure_file(TestPDALReader.cxx.in ${test_source})
  set(VTK_PDAL_READER_TESTS ${VTK_PDAL_READER_TESTS} ${test_source} PARENT_SCOPE)
endfunction()

add_vtk_pdal_reader_test(test_1.las 0)
add_vtk_pdal_reader_test(test_2.las -90)

vtk_add_test_cxx(vtkIOPDALCxxTests tests
  ${VTK_PDAL_READER_TESTS}
  )
vtk_test_cxx_executable(vtkIOPDALCxxTests tests)
