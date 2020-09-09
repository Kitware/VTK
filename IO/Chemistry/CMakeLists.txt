set(classes
  vtkGaussianCubeReader
  vtkMoleculeReaderBase
  vtkPDBReader
  vtkXYZMolReader
  vtkXYZMolReader2
  vtkCMLMoleculeReader
  vtkVASPAnimationReader
  vtkVASPTessellationReader
)

#set(private_classes )

vtk_module_add_module(VTK::IOChemistry
  CLASSES ${classes}
  #PRIVATE_CLASSES ${private_classes}
)
