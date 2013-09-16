set(Module_SRCS
  vtkEnSightWriter.cxx
  # vtkExodusIIWriter.cxx needs vtkMultiProcessController
  vtkPChacoReader.cxx
  vtkPDataSetReader.cxx
  vtkPDataSetWriter.cxx
  vtkPImageWriter.cxx
  vtkPOpenFOAMReader.cxx
  vtkPSLACReader.cxx
  vtkXMLPHierarchicalBoxDataWriter.cxx
  vtkXMLPMultiBlockDataWriter.cxx
  vtkXMLPUniformGridAMRWriter.cxx
  )

vtk_module_library(vtkIOParallel ${Module_SRCS})
target_link_libraries(vtkIOParallel LINK_PRIVATE vtkNetCDF_cxx)
