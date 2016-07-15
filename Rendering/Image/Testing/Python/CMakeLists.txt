vtk_add_test_python(
  TestDepthImageToPointCloud.py
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_python(
  TestDepthImageToPointCloud-TwoInputs.py
  )
endif()
