## Improved vtkTGAReader::CanReadFile validation

`vtkTGAReader::CanReadFile` now checks the bits-per-pixel field (byte 16)
in the TGA header in addition to the image type field (byte 2), reducing
false positives when reading non-TGA files.
