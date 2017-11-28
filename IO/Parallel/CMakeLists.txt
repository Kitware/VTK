set(Module_SRCS
  vtkEnSightWriter.cxx
  # vtkExodusIIWriter.cxx needs vtkMultiProcessController
  vtkMultiBlockPLOT3DReader.cxx
  vtkMultiBlockPLOT3DReaderInternals.cxx
  vtkPlot3DMetaReader.cxx
  vtkPChacoReader.cxx
  vtkPDataSetReader.cxx
  vtkPDataSetWriter.cxx
  vtkPImageWriter.cxx
  vtkPOpenFOAMReader.cxx
  vtkPSLACReader.cxx
  )

vtk_module_library(vtkIOParallel ${Module_SRCS})
