
vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestVRMLImporter.cxx,NO_VALID
  TestVRMLNormals.cxx
  )

list(APPEND tests TestOBJImporter.cxx)

ExternalData_add_test(VTKData
  NAME vtkIOImportCxx-OBJImport-SolidAndTextured
  COMMAND vtkIOImportCxxTests TestOBJImporter
    DATA{../Data/Input/cube-scene.obj}
    DATA{../Data/Input/cube-scene.mtl}
    DATA{../Data/Input/noise.png}
    -V DATA{../Data/Baseline/OBJImport-SolidAndTextured.png}
    -T "${VTK_TEST_OUTPUT_DIR}"
  )

vtk_test_cxx_executable(vtkIOImportCxxTests tests)

include(vtkOpenGL)

vtk_module_test_executable(vtkimportobj OBJImporter.cxx)
#  add_executable(vtkimportobj MACOSX_BUNDLE OBJImporter.cxx)
#  target_link_libraries(vtkimportobj LINK_PRIVATE
#      vtkIOXML vtkIOImport vtkRenderingOpenGL2)
vtk_opengl_link(vtkimportobj)

ExternalData_add_test(VTKData
  NAME vtkIOImportCxx-OBJImport-MixedOrder1
  COMMAND vtkimportobj
    "${VTK_TEST_OUTPUT_DIR}"
    DATA{../Data/Input/boxes_2.obj}
    DATA{../Data/Input/boxes_2.obj.mtl}
    DATA{../Data/Input/flare.jpg}
  )

ExternalData_add_test(VTKData
  NAME vtkIOImportCxx-OBJImport-NoMTL
  COMMAND vtkimportobj
    "${VTK_TEST_OUTPUT_DIR}"
    DATA{../Data/Input/boxes_3_no_mtl.obj}
  )

ExternalData_add_test(VTKData
  NAME vtkIOImportCxx-OBJImport-MTLwithoutTextureFile
  COMMAND vtkimportobj
    "${VTK_TEST_OUTPUT_DIR}"
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj}
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj.mtl}
  )
