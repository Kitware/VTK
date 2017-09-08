set(Module_SRCS
  vtkXMLPDataSetWriter.cxx
  vtkXMLPDataObjectWriter.cxx
  vtkXMLPDataWriter.cxx
  vtkXMLPImageDataWriter.cxx
  vtkXMLPPolyDataWriter.cxx
  vtkXMLPRectilinearGridWriter.cxx
  vtkXMLPStructuredDataWriter.cxx
  vtkXMLPStructuredGridWriter.cxx
  vtkXMLPTableWriter.cxx
  vtkXMLPUnstructuredDataWriter.cxx
  vtkXMLPUnstructuredGridWriter.cxx
  vtkXMLPHierarchicalBoxDataWriter.cxx
  vtkXMLPMultiBlockDataWriter.cxx
  vtkXMLPUniformGridAMRWriter.cxx
  )

set_source_files_properties(
  vtkXMLPDataWriter
  vtkXMLPStructuredDataWriter
  vtkXMLPUnstructuredDataWriter
  ABSTRACT
  )

vtk_module_library(vtkIOParallelXML ${Module_SRCS})
