# This is an alternative  way to add a named test that uses a common executable
# Use this if it does not require MPI

set(TEMP "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImporter-TwoTrianglesWithNegativeIndices,TestOBJImporter.cxx
    DATA{../Data/Input/TwoTrianglesWithNegativeIndices.obj}
    DATA{../Data/Input/TwoTrianglesWithNegativeIndices.mtl}
    DATA{../Data/Input/noise.png}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImport-SolidAndTextured,TestOBJImporter.cxx
    DATA{../Data/Input/cube-scene.obj}
    DATA{../Data/Input/cube-scene.mtl}
    DATA{../Data/Input/noise.png}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImport-mtllib-texturepath,TestOBJImporter.cxx
    DATA{../Data/Input/cube-scene.obj}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImport-MixedOrder1,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_2.obj}
    DATA{../Data/Input/boxes_2.obj.mtl}
    DATA{../Data/Input/flare.jpg}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImport-NoMTL,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_3_no_mtl.obj}
    DATA{../Data/Input/boxes_2.obj.mtl}
    DATA{../Data/Input/flare.jpg}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  OBJImport-MTLwithoutTextureFile,TestOBJImporter.cxx
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj}
    DATA{../Data/Input/boxes_4_mtl_no_texture.obj.mtl}
)

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
  TestOBJImporter-ScalarsDuplication,TestOBJImporter.cxx
    DATA{../Data/Input/TestOBJImporter-ScalarsDuplication.obj}
)

# Test a malformed file
vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestOBJImporter-Malformed,TestOBJImporter.cxx,NO_VALID
    DATA{../Data/Input/malformed.obj}
)

# Test an invalid file
vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestOBJImporter-Invalid,TestOBJImporter.cxx,NO_VALID
    DATA{../Data/Input/world_invalid.obj}
)

# Test an file with an invalid image
vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporter-Invalid,TestGLTFImporterInvalid.cxx,NO_VALID
    DATA{../Data/glTF/invalid.gltf}
    DATA{../Data/glTF/Duck0.bin}
)

# (vtk/vtk#19097) This test is skipped on wasm architecture.
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_tests_properties(VTK::IOImportCxx-TestOBJImporter-Malformed PROPERTIES PASS_REGULAR_EXPRESSION "Unexpected point indice value")
  set_tests_properties(VTK::IOImportCxx-TestOBJImporter-Malformed PROPERTIES FAIL_REGULAR_EXPRESSION "")
endif ()
  set_tests_properties(VTK::IOImportCxx-TestOBJImporter-Invalid PROPERTIES PASS_REGULAR_EXPRESSION "Error reading point with index")
  set_tests_properties(VTK::IOImportCxx-TestOBJImporter-Invalid PROPERTIES FAIL_REGULAR_EXPRESSION "")
  set_tests_properties(VTK::IOImportCxx-TestGLTFImporter-Invalid PROPERTIES PASS_REGULAR_EXPRESSION "Failed to open file")
  set_tests_properties(VTK::IOImportCxx-TestGLTFImporter-Invalid PROPERTIES FAIL_REGULAR_EXPRESSION "")

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestVRMLImporter.cxx,NO_VALID
  TestVRMLNormals.cxx
  )

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporter,TestGLTFImporter.cxx DATA{../Data/glTF/Avocado/Avocado.glb} 0 -1 1 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterStream,TestGLTFImporter.cxx DATA{../Data/glTF/Avocado/Avocado.glb} 1 -1 1 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  LOOSE_VALID
  TestGLTFImporterPBR,TestGLTFImporter.cxx DATA{../Data/glTF/WaterBottle/WaterBottle.glb} 0 -1 1 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  LOOSE_VALID
  TestGLTFImporterPBRStream,TestGLTFImporter.cxx DATA{../Data/glTF/WaterBottle/WaterBottle.glb} 1 -1 1 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterKHRLightsPunctual,TestGLTFImporter.cxx DATA{../Data/glTF/Lights/lights.gltf} 0 0 1 5 1 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterCamera,TestGLTFImporter.cxx DATA{../Data/glTF/Cameras/Cameras.gltf} 0 1 1 0 2 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterUnlit,TestGLTFImporter.cxx DATA{../Data/glTF/UnlitTest/UnlitTest.glb} 0 -1 2 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterArmature,TestGLTFImporter.cxx DATA{../Data/glTF/Armature.gltf} 0 -1 1 0 0 1
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterURITexture,TestGLTFImporter.cxx DATA{../Data/glTF/Lantern/Lantern.gltf} 0 -1 3 0 0 0
   DATA{../Data/glTF/Lantern/Lantern.bin}
   DATA{../Data/glTF/Lantern/Lantern_baseColor.png}
   DATA{../Data/glTF/Lantern/Lantern_emissive.png}
   DATA{../Data/glTF/Lantern/Lantern_normal.png}
   DATA{../Data/glTF/Lantern/Lantern_roughnessMetallic.png}
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterOpacity,TestGLTFImporter.cxx DATA{../Data/glTF/alpha.glb} 0 -1 16 0 0 0
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestGLTFImporterHierarchy,TestGLTFImporterHierarchy.cxx DATA{../Data/glTF/ABeautifulGame/ABeautifulGame.glb} -1
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  NO_VALID
  TestImportGLFTExportOBJ,TestImportExportOBJ.cxx
  DATA{../Data/glTF/Avocado/Avocado.glb}
  ${TEMP}/Avocado
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  NO_VALID
  TestImport3DSExportOBJ,TestImportExportOBJ.cxx
  DATA{../../../../Testing/Data/iflamigm.3ds}
  ${TEMP}/iflamigm
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestImport3DS,TestImportOBJ.cxx
  ${TEMP}/iflamigm.obj
  ${TEMP}/iflamigm.mtl
  ${TEMP}
)

set_tests_properties(VTK::IOImportCxx-TestImport3DS
      PROPERTIES
      DEPENDS
      "VTK::IOImportCxx-TestImport3DSExportOBJ"
)

vtk_add_test_cxx(vtkIOImportCxxTests tests
  TestImportGLFT,TestImportOBJ.cxx
  ${TEMP}/Avocado.obj
  ${TEMP}/Avocado.mtl
  ${TEMP}
)

set_tests_properties(VTK::IOImportCxx-TestImportGLFT
      PROPERTIES
      DEPENDS
      "VTK::IOImportCxx-TestImportGLFTExportOBJ"
)

vtk_test_cxx_executable(vtkIOImportCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY)
