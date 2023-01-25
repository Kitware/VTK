set(classes
  vtkEnSightWriter
  vtkMultiBlockPLOT3DReader
  vtkNek5000Reader
  vtkPChacoReader
  vtkPDataSetReader
  vtkPDataSetWriter
  vtkPImageWriter
  vtkPlot3DMetaReader
  vtkPOpenFOAMReader)

set(sources
  vtkMultiBlockPLOT3DReaderInternals.cxx)

set(private_headers
  vtkMultiBlockPLOT3DReaderInternals.h)

vtk_module_add_module(VTK::IOParallel
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
vtk_add_test_mangling(VTK::IOParallel)
