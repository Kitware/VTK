set (RenderingOpenGL2PythonTests
  TestUserShader2.py
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_python(
    ${RenderingOpenGL2PythonTests}
    )
endif ()
