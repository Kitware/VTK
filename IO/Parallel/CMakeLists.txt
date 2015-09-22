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

set_source_files_properties(
  vtkMultiBlockPLOT3DReaderInternals
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOParallel ${Module_SRCS})
