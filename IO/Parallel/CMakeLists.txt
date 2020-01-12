set(classes
  vtkEnSightWriter
  vtkMultiBlockPLOT3DReader
  vtkPlot3DMetaReader
  vtkPChacoReader
  vtkPDataSetReader
  vtkPDataSetWriter
  vtkPImageWriter
  vtkPOpenFOAMReader)

set(sources
  vtkMultiBlockPLOT3DReaderInternals.cxx)

set(private_headers
  vtkMultiBlockPLOT3DReaderInternals.h)

vtk_module_add_module(VTK::IOParallel
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
