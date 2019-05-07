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
  vtkGLTFDocumentLoader
  vtkGLTFReader
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

set(private_classes
  vtkGLTFDocumentLoaderInternals
  vtkGLTFUtils)

vtk_module_add_module(VTK::IOGeometry
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
