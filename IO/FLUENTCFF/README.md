# VTK::IOFLUENTCFF

This page describes the Fluent CFF IO functionality.

## vtkFLUENTCFFReader
Provide a reader for the FluentCFF file format.
Provide the Fluent CFF Reader (Common Fluid Format).

The reader supports cartesian grid, unstructured grid (poly, tetra, ...),
3D/2D, double and single precision files.

Similarly to the legacy reader (`vtkFLUENTReader`), the Fluent CFF reader
requires two files: the case file (`.cas.h5`) and the data file (`.dat.h5`).

The Fluent CFF readers uses the HDF library.

It is worth noting that the Fluent CFF file format is the default format in
the latest Fluent version and that ANSYS no longer uses the legacy binary or
ASCII formats.

Developed by Arthur Piquet and based on the vtkFLUENTReader class from Brian W. Dotson &
## Acknowledgments

Developed by Arthur Piquet and based on the `vtkFLUENTReader` class originally
developed from Brian W. Dotson & Terry E. Jordan (Department of Energy, National
Energy Technology Laboratory) and Douglas McCorkle (Iowa State University).
