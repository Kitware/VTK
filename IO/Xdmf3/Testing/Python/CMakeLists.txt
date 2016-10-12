vtk_add_test_python(
  VToXLoop.py,NO_VALID,NO_RT,NO_DATA
  ReadXdmfPolyhedron.py,NO_VALID,NO_RT
  )

if (VTK_USE_LARGE_DATA)
  set(_ManyFiles_ARGS "--do_big_files")
  set(_BigFiles "DATA{${VTK_TEST_INPUT_DIR}/XDMF/Big/,REGEX:.*}")
endif()
ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/XDMF/,REGEX:.*}"
  "${_BigFiles}"
  )
vtk_add_test_python(
  ManyFiles.py NO_OUTPUT NO_VALID NO_RT ${_ManyFiles_ARGS}
)
