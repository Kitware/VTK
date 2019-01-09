# TODO: Make FindGLUT use imported targets.
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})

vtk_add_test_cxx(vtkRenderingExternalCxxTests tests
  TestGLUTRenderWindow.cxx
  )

vtk_test_cxx_executable(vtkRenderingExternalCxxTests tests)
target_link_libraries(vtkRenderingExternalCxxTests PRIVATE
  ${GLUT_LIBRARY})
