ExternalData_add_test(VTKData
  NAME vtkIOImportCxx-OBJImport-SolidAndTextured
  COMMAND vtkIOImportCxxTests TestOBJImporter
    DATA{../Data/Input/cube-scene.obj}
    DATA{../Data/Input/cube-scene.mtl}
    DATA{../Data/Input/noise.png}
  )

include(vtkOpenGL)

vtk_module_test_executable(vtkimportobj OBJImporter.cxx)

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

# This is an alternative  way to add a named test that uses a common executable
# Use this if it does not require MPI
vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestOBJImporter-DuplicateMaps,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter.obj}
    DATA{../Data/Input/TestOBJImporter.mtl}
    DATA{../Data/Input/TestOBJImporter.png}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestOBJImporter-SharedMtl,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter-SharedMtl.obj}
    DATA{../Data/Input/TestOBJImporter-SharedMtl.mtl}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestOBJImporter-VtNoIndices,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.obj}
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.mtl}
    DATA{../Data/Input/TestOBJImporter-VtNoIndices.png}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestVRMLImporter.cxx,NO_VALID
  TestVRMLNormals.cxx
  )

vtk_test_cxx_executable(vtkIOImportCxxTests tests)
