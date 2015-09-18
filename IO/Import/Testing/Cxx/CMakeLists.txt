
set(TestOBJImporter_ARGS -D DATA{../Data/Input/boxes_1.obj}  DATA{../Data/Input/boxes_1.obj.mtl}
                            DATA{../Data/Input/flare.jpg}  DATA{../Data/Input/map1024.png})

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOBJImporter.cxx,NO_VALID,NO_OUTPUT
  TestVRMLNormals.cxx
  TestVRMLImporter.cxx,NO_VALID
  )


ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-OBJImport-MixedOrder1
  COMMAND ${vtk-module}CxxTests TestOBJImporter
    -D DATA{../Data/Input/boxes_2.obj}  DATA{../Data/Input/boxes_2.obj.mtl}
       DATA{../Data/Input/flare.jpg}    DATA{../Data/Input/map1024.png}
    -D " ${VTK_TEST_OUTPUT_DIR} "
  )

ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-OBJImport-NoMTL
  COMMAND ${vtk-module}CxxTests TestOBJImporter
    -D DATA{../Data/Input/boxes_3_no_mtl.obj}
    -D " ${VTK_TEST_OUTPUT_DIR} "
  )

ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-OBJImport-SolidAndTextured
  COMMAND ${vtk-module}CxxTests TestOBJImporter
    -D DATA{../Data/Input/cube-scene.obj}  DATA{../Data/Input/cube-scene.mtl}
       DATA{../Data/Input/noise.png}       DATA{../Data/Input/noise.png}
    -V DATA{../Data/Baseline/OBJImport-SolidAndTextured.png}
    -T "${VTK_TEST_OUTPUT_DIR}"
  )

ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-OBJImport-MTLwithoutTextureFile
  COMMAND ${vtk-module}CxxTests TestOBJImporter
    -D DATA{../Data/Input/boxes_4_mtl_no_texture.obj}  DATA{../Data/Input/boxes_4_mtl_no_texture.obj.mtl}
    -D " ${VTK_TEST_OUTPUT_DIR} "
  )


vtk_test_cxx_executable(${vtk-module}CxxTests tests)
