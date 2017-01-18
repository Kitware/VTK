set (ContextOpenGL2CxxTests
  TestPolyDataToContext.cxx
  )

# Generic tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestContext2D.cxx,NO_VALID
  )

# OpenGL2 specific tests
if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    ${ContextOpenGL2CxxTests}
    )
endif ()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
