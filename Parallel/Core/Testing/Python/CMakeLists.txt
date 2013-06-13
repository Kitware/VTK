add_test_python(TestPolyDataPieces.py Parallel)
if (VTK_DATA_ROOT)
  add_test_python(TestUnstructuredPieces.py Parallel)
endif()
