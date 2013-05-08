if(VTK_DATA_ROOT})
  vtk_add_test_python(TestGlobFileNames.py ${VTK_DATA_ROOT})
  vtk_add_test_python(TestSortFileNames.py ${VTK_DATA_ROOT})
endif()
