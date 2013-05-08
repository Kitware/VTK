if(VTK_DATA_ROOT)
  vtk_add_test_python(MeshQuality.py ${VTK_DATA_ROOT} NO_RT)
endif(VTK_DATA_ROOT)
