
set(TestOBJImporter_ARGS -D DATA{../Data/Input/boxes_1.obj}  DATA{../Data/Input/boxes_1.obj.mtl}
                            DATA{../Data/Input/flare.jpg}  DATA{../Data/Input/map1024.png})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOBJImporter.cxx,NO_VALID,NO_OUTPUT
  TestVRMLNormals.cxx
  TestVRMLImporter.cxx,NO_VALID
  )


#vtk_add_test_cxx(${vtk-module}CxxTests tests TestObjImporter.cxx,NO_VALID,NO_OUTPUT )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
