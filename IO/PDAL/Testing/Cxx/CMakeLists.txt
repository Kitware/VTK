set(VTK_Pdal_READER_TESTS)
function(add_vtk_pdal_reader_test test_input elevation)
  get_filename_component(VTK_Pdal_READER_POSTFIX ${test_input} NAME_WE)
  string(REPLACE "-" "_" VTK_Pdal_READER_POSTFIX ${VTK_Pdal_READER_POSTFIX})
  set(VTK_Pdal_READER_TEST_INPUT "${test_input}")
  set(VTK_Pdal_READER_ELEVATION "${elevation}")
  set(test_source "TestPdalReader_${VTK_Pdal_READER_POSTFIX}.cxx")
  configure_file(TestPdalReader.cxx.in ${test_source})
  set(VTK_Pdal_READER_TESTS ${VTK_Pdal_READER_TESTS} ${test_source} PARENT_SCOPE)
endfunction()

add_vtk_pdal_reader_test(test_1.las 0)
add_vtk_pdal_reader_test(test_2.las -90)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${VTK_Pdal_READER_TESTS}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
