## vtkWriter::Write() now return the correct and other changes

vtkWriter::Write() now return false if the writer failed to write
but did not set an error code.

In that context, vtkWriter::WriteData has been deprecated in favor of vtkWriter::WriteDataAndReturn
