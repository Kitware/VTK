## Adding stream support to various readers

Stream support have been added to:
 - vtkSTLReader
 - vtkPTSReader
 - vtkHDFReader (prefer memory stream)
 - vtkCityGMLReader (prefer memory stream)

In that context the following protected methods are now private/removed:
 - vtkSTLReader::ReadBinarySTL
 - vtkSTLReader::ReadASCIISTL
 - vtkSTLReader::GetSTLFileType
