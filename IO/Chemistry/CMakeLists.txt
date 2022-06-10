set(classes
  vtkCMLMoleculeReader
  vtkGaussianCubeReader
  vtkGaussianCubeReader2
  vtkMoleculeReaderBase
  vtkPDBReader
  vtkVASPAnimationReader
  vtkVASPTessellationReader
  vtkXYZMolReader
  vtkXYZMolReader2
)

vtk_module_add_module(VTK::IOChemistry
  CLASSES ${classes}
)
vtk_add_test_mangling(VTK::IOChemistry)
