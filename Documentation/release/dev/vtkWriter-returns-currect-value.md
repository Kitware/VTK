## vtkWriter::Write() now return the correct value

vtkWriter::Write() now return false if the writer failed to write
but did not set an error code.
