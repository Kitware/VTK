
## Added vector overrun detection for vtkBMPReader and better overall protection

Added simple vector overrun detection to the `vtkBMPReader`. Additionally when a ExecuteInformation() fails ExecuteDataWithInformation() will not be attempted with garbled data. This ensures that when reading a BMP file that is invalid (e.g. garbled header or cut-off file) the reader won't try to read data past the end of the file contents vector.
