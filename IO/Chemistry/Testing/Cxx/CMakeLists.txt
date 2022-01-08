
set(TestVASPAnimationReader_ARGS DATA{../Data/VASP/NPT_Z_ANIMATE.out})
set(TestVASPTessellationReader_ARGS DATA{../Data/VASP/NPT_Z_TESSELLATE.out})

vtk_add_test_cxx(vtkIOChemistryCxxTests tests
  TestCMLMoleculeReader.cxx
  TestVASPAnimationReader.cxx
  TestVASPTessellationReader.cxx
)

vtk_add_test_cxx(vtkIOChemistryCxxTests tests
  TestPDBReader.cxx NO_VALID DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/6VWW.pdb}
)

vtk_test_cxx_executable(vtkIOChemistryCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
)
