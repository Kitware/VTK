## vtkImageReader2 now supports reading vtkResourceStream

vtkImageReader2::Get/SetStream methods have been added and implemented in vtkPNGReader, vtkTGAReader and vtkJPEGReader.

vtkImageReader2::SetMemoryBuffer have been deprecated in favor of SetStream.
