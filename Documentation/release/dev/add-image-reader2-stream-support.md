## vtkImageReader2 now supports reading vtkResourceStream

vtkImageReader2::Get/SetStream methods have been added and implemented in:
 - vtkPNGReader
 - vtkTGAReader
 - vtkJPEGReader
 - vtkHDRReader
 - vtkBMPReader

vtkImageReader2::SetMemoryBuffer have been deprecated in favor of SetStream.

In that context the following protected methods are now private/removed:
 - vtkHDRReader::ReadAllFileNoRLE
 - vtkHDRReader::ReadLineRLE
