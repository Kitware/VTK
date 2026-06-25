## Stricter `CanReadFile` validation for `vtkTGAReader`

`vtkTGAReader::CanReadFile` now performs a multi-layer validation to eliminate
false positives caused by non-TGA files (e.g. ASCII point-cloud `.pts` files)
whose bytes happened to satisfy the previous two-field check.

The enhanced logic first looks for the unambiguous TGA 2.0 footer signature
(`TRUEVISION-XFILE.`) at the end of the file. For TGA 1.0 files that lack this
footer, the 18-byte header is cross-validated against five additional
constraints: color-map type must be 0, image width and height must be non-zero,
descriptor reserved bits must be clear, and the file must be large enough to
hold the declared pixel payload (uncompressed images only).
