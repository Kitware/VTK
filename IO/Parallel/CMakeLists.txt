find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkEnSightWriter.cxx
  # vtkExodusIIWriter.cxx needs vtkMultiProcessController
  vtkPChacoReader.cxx
  vtkPDataSetReader.cxx
  vtkPDataSetWriter.cxx
  vtkPImageWriter.cxx
  vtkPNrrdReader.cxx
  vtkPOpenFOAMReader.cxx
  vtkPSLACReader.cxx
  vtkVPICReader.cxx
  vtkWindBladeReader.cxx
  vtkXMLPHierarchicalBoxDataWriter.cxx
  vtkXMLPMultiBlockDataWriter.cxx
  )

vtk_module_library(vtkIOParallel ${Module_SRCS})
