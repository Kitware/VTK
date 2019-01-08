set(classes
  vtkAVSucdReader
  vtkBYUReader
  vtkBYUWriter
  #vtkCGMWriter # Needs vtkViewport.
  vtkChacoReader
  vtkFacetWriter
  vtkFLUENTReader
  vtkGAMBITReader
  vtkGaussianCubeReader
  vtkHoudiniPolyDataWriter
  vtkIVWriter
  vtkMCubesReader
  vtkMCubesWriter
  vtkMFIXReader
  vtkMoleculeReaderBase
  vtkOBJReader
  vtkOBJWriter
  vtkOpenFOAMReader
  vtkParticleReader
  vtkPDBReader
  vtkProStarReader
  vtkPTSReader
  vtkSTLReader
  vtkSTLWriter
  vtkTecplotReader
  vtkWindBladeReader
  vtkXYZMolReader)

vtk_module_add_module(VTK::IOGeometry
  CLASSES ${classes})
