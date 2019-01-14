# TODO: Simplify this with some argument passing.
set(VTK_LAS_READER_TESTS)
function(add_vtk_las_reader_test test_input elevation)
  get_filename_component(VTK_LAS_READER_POSTFIX ${test_input} NAME_WE)
  set(VTK_LAS_READER_TEST_INPUT "${test_input}")
  set(VTK_LAS_READER_ELEVATION "${elevation}")
  set(test_source "TestLASReader_${VTK_LAS_READER_POSTFIX}.cxx")
  configure_file(TestLASReader.cxx.in ${test_source})
  set(VTK_LAS_READER_TESTS ${VTK_LAS_READER_TESTS} ${test_source} PARENT_SCOPE)
endfunction()

add_vtk_las_reader_test(test_1.las 0)
add_vtk_las_reader_test(test_2.las -90)

vtk_add_test_cxx(vtkIOLASCxxTests tests
  ${VTK_LAS_READER_TESTS}
  )
vtk_test_cxx_executable(vtkIOLASCxxTests tests)
