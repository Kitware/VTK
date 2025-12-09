## Adding stream support to various readers

vtkResourceStream can now be encapsulated into a std::streambuf using
vtkResourceStream::ToStreambuf().

Stream support have been added to:
 - vtkSTLReader
 - vtkPTSReader
 - vtkHDFReader (prefer memory stream)
 - vtkCityGMLReader (prefer memory stream)
 - vtkOpenVDBReader
 - vtkDICOMImageReader

In that context the following protected methods are now private/removed:
 - vtkSTLReader::ReadBinarySTL
 - vtkSTLReader::ReadASCIISTL
 - vtkSTLReader::GetSTLFileType
