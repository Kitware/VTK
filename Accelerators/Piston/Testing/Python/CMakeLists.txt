#
# Exercise origin and spacing aspect of vtkImageData
#
set(vtk_test_prefix Placed)
vtk_add_test_python(
  NO_OUTPUT NO_RT
  TestContour.py
  TestThreshold.py
  TestSlice.py
  TestSort.py
  )
unset(vtk_test_prefix)

#
# Exercise thrust to gl direct on GPU rendering.
#
set(vtk_test_prefix Direct)
set(Rendering_ARGS --gpu_render)
vtk_add_test_python(
  NO_RT NO_OUTPUT
  TestRendering.py
  )
unset(vtk_test_prefix)

#
# Test base functionality.
#
set(python_tests
  TestConversion
  TestContour
  TestRendering
  TestThreshold
  TestSlice
  TestSort
  )
set(test_files)
foreach(test IN LISTS python_tests)
  set(${test}_ARGS --normalize)
  list(APPEND test_files ${test}.py)
endforeach()
vtk_add_test_python(
  NO_RT NO_VALID
  ${test_files}
  )
