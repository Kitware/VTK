## Added vector overrun detection for vtkTGAReader

Added simple vector overrun detection to the `vtkTGAReader`. This ensures that when reading a TGA file that is invalid (e.g. garbled header or cut-off file) the reader won't try to read data past the end of the file contents vector.
