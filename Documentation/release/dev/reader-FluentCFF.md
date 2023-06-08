## FluentCFFReader: Add ANSYS Fluent CFF (Common Fluid Format) reader

Add the Fluent CFF Reader (Common Fluid Format) into its own dedicated module VTK::IOFLUENTCFF.
This file format is the default format in the latest Fluent version. The legacy format
(Binary or ASCII) is not used anymore by ANSYS. The CFF format uses the HDF library. Similarly to
the legacy Fluent format, it needs 2 files : a case file (extension '.cas.h5') and a data file (extension '.dat.h5').
It works with Cartesian grid, Unstructured grid (poly, tetra, ...), 3D/2D, double and single precision files.

Online doc: https://docs.vtk.org/en/latest/modules/vtk-modules/IO/FLUENTCFF/README.html
