if(VTK_PYTHON_EXE AND VTK_DATA_ROOT)
  vtk_add_test_python(TestTkRenderWidget.py NO_RT)
  vtk_add_test_python(TestTkRenderWindowInteractor.py NO_RT)
endif()
