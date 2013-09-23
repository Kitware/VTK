SET(Module_SRCS
  vtkCompositeDataReader.cxx
  vtkCompositeDataWriter.cxx
  vtkDataObjectReader.cxx
  vtkDataObjectWriter.cxx
  vtkDataReader.cxx
  vtkDataSetReader.cxx
  vtkDataSetWriter.cxx
  vtkDataWriter.cxx
  vtkGenericDataObjectReader.cxx
  vtkGenericDataObjectWriter.cxx
  vtkGraphReader.cxx
  vtkGraphWriter.cxx
  vtkPixelExtentIO.cxx
  vtkPolyDataReader.cxx
  vtkPolyDataWriter.cxx
  vtkRectilinearGridReader.cxx
  vtkRectilinearGridWriter.cxx
  vtkSimplePointsReader.cxx
  vtkSimplePointsWriter.cxx
  vtkStructuredGridReader.cxx
  vtkStructuredGridWriter.cxx
  vtkStructuredPointsReader.cxx
  vtkStructuredPointsWriter.cxx
  vtkTableReader.cxx
  vtkTableWriter.cxx
  vtkTreeReader.cxx
  vtkTreeWriter.cxx
  vtkUnstructuredGridReader.cxx
  vtkUnstructuredGridWriter.cxx
  )

set_source_files_properties(
  vtkPixelExtentIO.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOLegacy ${Module_SRCS})
