set(classes
  vtkAVSucdReader
  vtkBYUReader
  vtkBYUWriter
  #vtkCGMWriter # Needs vtkViewport.
  vtkChacoReader
  vtkFacetWriter
  vtkFLUENTReader
  vtkGAMBITReader
  vtkGLTFDocumentLoader
  vtkGLTFReader
  vtkGLTFWriter
  vtkHoudiniPolyDataWriter
  vtkIVWriter
  vtkMCubesReader
  vtkMCubesWriter
  vtkMFIXReader
  vtkOBJReader
  vtkOBJWriter
  vtkOpenFOAMReader
  vtkParticleReader
  vtkProStarReader
  vtkPTSReader
  vtkSTLReader
  vtkSTLWriter
  vtkTecplotReader
  vtkWindBladeReader)

set(private_classes
  vtkGLTFDocumentLoaderInternals
  vtkGLTFWriterUtils
  vtkGLTFUtils)

vtk_module_add_module(VTK::IOGeometry
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
