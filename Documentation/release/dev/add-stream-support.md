## Adding stream support to vtkSTLReader

vtkSTLReader new supports reading streams

In that context the following protected methods are now private/removed:
 - vtkSTLReader::ReadBinarySTL
 - vtkSTLReader::ReadASCIISTL
 - vtkSTLReader::GetSTLFileType
