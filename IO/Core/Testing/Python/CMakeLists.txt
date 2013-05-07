if(VTK_DATA_ROOT})
  add_test_python(TestGlobFileNames.py ${VTK_DATA_ROOT})
  add_test_python(TestSortFileNames.py ${VTK_DATA_ROOT})
endif()
