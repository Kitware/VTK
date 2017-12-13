set(Module_SRCS
  vtkRTXMLPolyDataReader.cxx
  vtkXMLCompositeDataReader.cxx
  vtkXMLCompositeDataWriter.cxx
  vtkXMLDataReader.cxx
  vtkXMLDataSetWriter.cxx
  vtkXMLDataObjectWriter.cxx
  vtkXMLFileReadTester.cxx
  vtkXMLGenericDataObjectReader.cxx
  vtkXMLHierarchicalBoxDataFileConverter.cxx
  vtkXMLHierarchicalBoxDataReader.cxx
  vtkXMLHierarchicalBoxDataWriter.cxx
  vtkXMLHierarchicalDataReader.cxx
  vtkXMLImageDataReader.cxx
  vtkXMLImageDataWriter.cxx
  vtkXMLMultiBlockDataReader.cxx
  vtkXMLMultiBlockDataWriter.cxx
  vtkXMLMultiGroupDataReader.cxx
  vtkXMLPDataReader.cxx
  vtkXMLPDataObjectReader.cxx
  vtkXMLPImageDataReader.cxx
  vtkXMLPolyDataReader.cxx
  vtkXMLPolyDataWriter.cxx
  vtkXMLPPolyDataReader.cxx
  vtkXMLPRectilinearGridReader.cxx
  vtkXMLPStructuredDataReader.cxx
  vtkXMLPStructuredGridReader.cxx
  vtkXMLPTableReader.cxx
  vtkXMLPUnstructuredDataReader.cxx
  vtkXMLPUnstructuredGridReader.cxx
  vtkXMLReader.cxx
  vtkXMLRectilinearGridReader.cxx
  vtkXMLRectilinearGridWriter.cxx
  vtkXMLStructuredDataReader.cxx
  vtkXMLStructuredDataWriter.cxx
  vtkXMLStructuredGridReader.cxx
  vtkXMLStructuredGridWriter.cxx
  vtkXMLTableReader.cxx
  vtkXMLTableWriter.cxx
  vtkXMLUniformGridAMRReader.cxx
  vtkXMLUniformGridAMRWriter.cxx
  vtkXMLUnstructuredDataReader.cxx
  vtkXMLUnstructuredDataWriter.cxx
  vtkXMLUnstructuredGridReader.cxx
  vtkXMLUnstructuredGridWriter.cxx
  vtkXMLWriterC.cxx
  vtkXMLWriter.cxx
  )

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkXMLHyperOctreeReader.cxx
    vtkXMLHyperOctreeWriter.cxx
  )
endif()

set_source_files_properties(
  vtkXMLCompositeDataReader
  vtkXMLCompositeDataWriter
  vtkXMLDataReader
  vtkXMLPDataReader
  vtkXMLPStructuredDataReader
  vtkXMLPUnstructuredDataReader
  vtkXMLReader
  vtkXMLStructuredDataReader
  vtkXMLStructuredDataWriter
  vtkXMLUnstructuredDataReader
  vtkXMLUnstructuredDataWriter
  vtkXMLWriter
  ABSTRACT
  )

set_source_files_properties(
  vtkXMLWriterC
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOXML ${Module_SRCS})
