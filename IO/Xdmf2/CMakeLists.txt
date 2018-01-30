set(Module_SRCS
  vtkSILBuilder.cxx
  vtkXdmfDataArray.cxx
  vtkXdmfHeavyData.cxx
  vtkXdmfReader.cxx
  vtkXdmfReaderInternal.cxx
  vtkXdmfWriter.cxx
  )

#set(${vtk-module}_NO_HeaderTest 1) # TODO: helper classes and re-enable
vtk_module_library(vtkIOXdmf2 ${Module_SRCS})
