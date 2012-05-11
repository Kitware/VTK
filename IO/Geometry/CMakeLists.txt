SET(Module_SRCS
  vtkAVSucdReader.cxx
  vtkBYUReader.cxx
  vtkBYUWriter.cxx
  #vtkCGMWriter.cxx # Needs vtkViewport.
  vtkChacoReader.cxx
  vtkCompositeDataReader.cxx
  vtkCompositeDataWriter.cxx
  vtkDataSetReader.cxx
  vtkDataSetWriter.cxx
  vtkFacetWriter.cxx
  vtkFLUENTReader.cxx
  vtkGAMBITReader.cxx
  vtkGenericDataObjectReader.cxx
  vtkGenericDataObjectWriter.cxx
  vtkGenericMovieWriter.cxx
  vtkIVWriter.cxx
  vtkMaterialLibrary.cxx
  vtkMCubesReader.cxx
  vtkMCubesWriter.cxx
  vtkMFIXReader.cxx
  vtkMoleculeReaderBase.cxx
  vtkMultiBlockPLOT3DReader.cxx
  vtkOBJReader.cxx
  vtkOpenFOAMReader.cxx
  vtkParticleReader.cxx
  vtkPDBReader.cxx
  vtkPolyDataReader.cxx
  vtkPolyDataWriter.cxx
  vtkProStarReader.cxx
  vtkRectilinearGridReader.cxx
  vtkRectilinearGridWriter.cxx
  vtkSESAMEReader.cxx
  vtkSimplePointsReader.cxx
  vtkSimplePointsWriter.cxx
  vtkSTLReader.cxx
  vtkSTLWriter.cxx
  vtkStructuredGridReader.cxx
  vtkStructuredGridWriter.cxx
  vtkTecplotReader.cxx
  vtkTreeReader.cxx
  vtkTreeWriter.cxx
  vtkUGFacetReader.cxx
  vtkUnstructuredGridReader.cxx
  vtkUnstructuredGridWriter.cxx

  vtkStructuredPointsReader.cxx
  vtkStructuredPointsWriter.cxx

  vtkShaderCodeLibrary.cxx

  vtkXYZMolReader.cxx
  )

set_source_files_properties(
  vtkGenericMovieWriter
  vtkMoleculeReaderBase
  ABSTRACT
  )

vtk_module_library(vtkIOGeometry ${Module_SRCS})
