if(VTK_PYTHON_EXE)
  vtk_add_test_python(TestTkRenderWidget.py NO_RT)
  vtk_add_test_python(TestTkRenderWindowInteractor.py NO_RT)
endif()
