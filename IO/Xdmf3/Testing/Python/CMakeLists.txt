set(vtk_test_prefix XML)
set(VToXLoop_ARGS -ldl=1000)
vtk_add_test_python(
  VToXLoop.py NO_OUTPUT NO_VALID NO_RT NO_DATA
  )
unset(vtk_test_prefix)

set(vtk_test_prefix H5)
set(VToXLoop_ARGS -ldl=0)
vtk_add_test_python(
  VToXLoop.py NO_OUTPUT NO_VALID NO_RT NO_DATA
  )
unset(vtk_test_prefix)

if (VTK_USE_LARGE_DATA)
  set(_ManyFiles_ARGS "--do_big_files")
endif()
vtk_add_test_python(
  ManyFiles.py NO_OUTPUT NO_VALID NO_RT ${_ManyFiles_ARGS}
)
