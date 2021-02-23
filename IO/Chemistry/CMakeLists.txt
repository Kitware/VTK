set(classes
  vtkCMLMoleculeReader
  vtkGaussianCubeReader
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
