set(classes
  vtkXMLCompositeDataSetWriterHelper
  vtkXMLDataWriterHelper
  vtkXMLPartitionedDataSetCollectionWriter
  vtkXMLPartitionedDataSetWriter
  vtkXMLWriter2
  vtkXMLPDataObjectWriter
  vtkXMLPDataSetWriter
  vtkXMLPDataWriter
  vtkXMLPHierarchicalBoxDataWriter
  vtkXMLPHyperTreeGridWriter
  vtkXMLPImageDataWriter
  vtkXMLPMultiBlockDataWriter
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
