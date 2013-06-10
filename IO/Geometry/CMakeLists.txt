SET(Module_SRCS
  vtkAVSucdReader.cxx
  vtkBYUReader.cxx
  vtkBYUWriter.cxx
  #vtkCGMWriter.cxx # Needs vtkViewport.
  vtkChacoReader.cxx
  vtkFacetWriter.cxx
  vtkFLUENTReader.cxx
  vtkGAMBITReader.cxx
  vtkGaussianCubeReader.cxx
  vtkIVWriter.cxx
  vtkMCubesReader.cxx
  vtkMCubesWriter.cxx
  vtkMFIXReader.cxx
  vtkMoleculeReaderBase.cxx
  vtkMultiBlockPLOT3DReader.cxx
  vtkMultiBlockPLOT3DReaderInternals.cxx
  vtkOBJReader.cxx
  vtkOpenFOAMReader.cxx
  vtkParticleReader.cxx
  vtkPDBReader.cxx
  vtkPlot3DMetaReader.cxx
  vtkProStarReader.cxx
  vtkSESAMEReader.cxx
  vtkSTLReader.cxx
  vtkSTLWriter.cxx
  vtkTecplotReader.cxx
  vtkUGFacetReader.cxx
  vtkWindBladeReader.cxx
  vtkXYZMolReader.cxx
  )

set_source_files_properties(
  vtkMoleculeReaderBase
  ABSTRACT
  )

set_source_files_properties(
  vtkMultiBlockPLOT3DReaderInternals
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOGeometry ${Module_SRCS})
