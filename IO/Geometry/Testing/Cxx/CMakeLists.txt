vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestDataObjectIO.cxx,NO_VALID
  TestIncrementalOctreePointLocator.cxx,NO_VALID
  UnstructuredGridCellGradients.cxx
  UnstructuredGridFastGradients.cxx
  UnstructuredGridGradients.cxx
  TestOBJPolyDataWriter.cxx
  TestOBJReaderComments.cxx,NO_VALID
  TestOBJReaderGroups.cxx,NO_VALID
  TestOBJReaderMaterials.cxx,NO_VALID
  TestOBJReaderMultiTexture.cxx,NO_VALID
  TestOBJReaderNormalsTCoords.cxx,NO_VALID
  TestOBJReaderRelative.cxx,NO_VALID
  TestOBJReaderSingleTexture.cxx,NO_VALID
  TestOpenFOAMReader.cxx
  TestOpenFOAMReader64BitFloats.cxx
  TestOpenFOAMReaderRegEx.cxx,NO_VALID
  TestProStarReader.cxx
  TestTecplotReader.cxx
  TestAMRReadWrite.cxx,NO_VALID
  TestSimplePointsReaderWriter.cxx,NO_VALID
  TestHoudiniPolyDataWriter.cxx,NO_VALID
  UnitTestSTLWriter.cxx,NO_VALID
  )

LIST(APPEND tecplotFiles
    3dgeom.dat
    block_febrick_3d.dat
    block_fetet_3d.dat
    channel.dat
    crystal_plot.dat
    cube.dat
    cylinder_3d.dat
    cylinder_slice.dat
    cylindrical.dat
    ell.dat
    humanoid_quad.dat
    humanoid_tri.dat
    movie.dat
    multzn2d.dat
    plane_slice.dat
    point_febrick_3d_01.dat
    point_febrick_3d_02.dat
    point_fequad_2d.dat
    point_fetet_3d.dat
    point_fetri_2d_01.dat
    point_fetri_2d_03.dat
    simp3dbk.dat
    simp3dpt.dat
    simpscat.dat
    simpxy.dat
    simpxy2.dat
    tecplot_fepolygon.dat
    tecplot_fepolyhedron.dat
    tiny.dat
)
foreach(item IN LISTS tecplotFiles)
    get_filename_component(name "${item}" NAME_WE)

    vtk_add_test_cxx(vtkIOGeometryCxxTests tests
      TestTecplotReader2_${name},TestTecplotReader2.cxx ${item} NO_VALID NO_OUTPUT)

endforeach()

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestSTLReaderSinglePatch,TestSTLReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/42400-IDGH.stl}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestSTLReaderMultiplePatches,TestSTLReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/multiple_patches.stl}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestPTSReader,TestPTSReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/samplePTS.pts}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestAVSucdReaderContiguousASCII,TestAVSucdReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/cellsnd.ascii.inp}
)
vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestAVSucdReaderNonContiguousASCII,TestAVSucdReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/cellsnd.noncontiguous.ascii.inp}
)
vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestAVSucdReaderBinary,TestAVSucdReader.cxx DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/cellsnd.bin.inp}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGeometry,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/Triangle/Triangle.gltf,simpleTriangle.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderEmbeddedGeometry,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/Triangle/EmbeddedTriangle.gltf}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGLBGeometry,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/Triangle/Triangle.glb}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderAnimation,TestGLTFReaderAnimation.cxx 200 DATA{../Data/glTF/BoxAnimated/BoxAnimated.gltf,BoxAnimated0.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderMorphing,TestGLTFReaderAnimation.cxx 200 DATA{../Data/glTF/AnimatedMorphCube/AnimatedMorphCube.gltf,AnimatedMorphCube.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderSkinning,TestGLTFReaderAnimation.cxx 200 DATA{../Data/glTF/SimpleSkin/SimpleSkin.gltf}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderMultipleAnimations,TestGLTFReaderAnimation.cxx 75 DATA{../Data/glTF/InterpolationTest/InterpolationTest.gltf,interpolation.bin,InterpolationTest_data.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGeneratePointIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_POINTS.gltf,Mesh_PrimitiveMode_00.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadPointIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_POINTS_2.gltf,Mesh_PrimitiveMode_07.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGenerateLineIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINES.gltf,Mesh_PrimitiveMode_01.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadLineIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINES_2.gltf,Mesh_PrimitiveMode_08.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGenerateLineLoopIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINE_LOOP.gltf,Mesh_PrimitiveMode_02.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadLineLoopIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINE_LOOP_2.gltf,Mesh_PrimitiveMode_09.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGenerateLineStripIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINE_STRIP.gltf,Mesh_PrimitiveMode_03.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadLineStripIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_LINE_STRIP_2.gltf,Mesh_PrimitiveMode_10.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGenerateTriangleFanIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_TRIANGLE_FAN.gltf,Mesh_PrimitiveMode_05.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadTriangleFanIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_TRIANGLE_FAN_2.gltf,Mesh_PrimitiveMode_12.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderGenerateTriangleStripIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_TRIANGLE_STRIP.gltf,Mesh_PrimitiveMode_04.bin}
)

vtk_add_test_cxx(vtkIOGeometryCxxTests tests
  TestGLTFReaderReadTriangleStripIndices,TestGLTFReaderGeometry.cxx DATA{../Data/glTF/PrimitiveModes/Mesh_PrimitiveMode_TRIANGLE_STRIP_2.gltf,Mesh_PrimitiveMode_11.bin}
)

if(enable_large_data_tests)
  vtk_add_test_cxx(vtkIOGeometryCxxTests large_data_tests
    TestWindBladeReader.cxx
    TestMFIXReader.cxx,NO_VALID
    )
  list(APPEND tests
    ${large_data_tests}
    )
endif()

vtk_test_cxx_executable(vtkIOGeometryCxxTests tests)
