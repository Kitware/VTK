FIND_PACKAGE(GLUT REQUIRED)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLUT_INCLUDE_DIR}
)

VTK_ADD_TEST_CXX(vtkRenderingExternalCxxTests tests
  TestGLUTRenderWindow.cxx
  )

VTK_TEST_CXX_EXECUTABLE(vtkRenderingExternalCxxTests tests)
TARGET_LINK_LIBRARIES(vtkRenderingExternalCxxTests LINK_PRIVATE
  vtkRenderingExternal ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
include(vtkOpenGL)
vtk_opengl_link(vtkRenderingExternalCxxTests)
