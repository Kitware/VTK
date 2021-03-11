set(classes
  vtkXMLPDataObjectWriter
  vtkXMLPDataSetWriter
  vtkXMLPDataWriter
  vtkXMLPHierarchicalBoxDataWriter
  vtkXMLPHyperTreeGridWriter
  vtkXMLPImageDataWriter
  vtkXMLPMultiBlockDataWriter
  vtkXMLPPartitionedDataSetWriter
  vtkXMLPPolyDataWriter
  vtkXMLPRectilinearGridWriter
  vtkXMLPStructuredDataWriter
  vtkXMLPStructuredGridWriter
  vtkXMLPTableWriter
  vtkXMLPUniformGridAMRWriter
  vtkXMLPUnstructuredDataWriter
  vtkXMLPUnstructuredGridWriter)

vtk_module_add_module(VTK::IOParallelXML
  CLASSES ${classes})
