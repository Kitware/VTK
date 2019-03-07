set(classes
  vtkXMLPDataSetWriter
  vtkXMLPDataObjectWriter
  vtkXMLPDataWriter
  vtkXMLPImageDataWriter
  vtkXMLPPolyDataWriter
  vtkXMLPRectilinearGridWriter
  vtkXMLPStructuredDataWriter
  vtkXMLPStructuredGridWriter
  vtkXMLPTableWriter
  vtkXMLPUnstructuredDataWriter
  vtkXMLPUnstructuredGridWriter
  vtkXMLPHierarchicalBoxDataWriter
  vtkXMLPMultiBlockDataWriter
  vtkXMLPUniformGridAMRWriter
  vtkXMLPHyperTreeGridWriter)

vtk_module_add_module(VTK::IOParallelXML
  CLASSES ${classes})
