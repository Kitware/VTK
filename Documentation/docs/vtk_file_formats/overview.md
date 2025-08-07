# Overview

A lot of this material is taken from [The VTK User’s Guide](https://www.kitware.com/products/books/VTKUsersGuide.pdf).

The *Visualization Toolkit* provides a number of source and writer objects to read and write popular data file formats. The *Visualization Toolkit* also provides some of its own file formats. The main reason for creating yet another data file format is to offer a consistent data representation scheme for a variety of dataset types, and to provide a simple method to communicate data between software. Whenever possible, we recommend that you use formats that are more widely used. But if this is not possible, the *Visualization Toolkit* formats described here can be used instead. Note that these formats may not be supported by many other tools.

There are three different styles of file formats available in VTK: Legacy, XML and VTKHDF.


## Legacy

It's a serial formats that are easy to read and write either by hand or programmatically.

For more details, read the [related documentation](vtk_legacy_file_format.md).

## XML

More flexible but more complex than the legacy file format, it supports random access, parallel I/O, and portable data compression and are preferred to the serial VTK file formats whenever possible.

For more details, read the [related documentation](vtkxml_file_format.md).


## VTKHDF

This is a file format using the same concepts as the XML formats described above but relying on HDF5 for actual storage. It is simpler than the XML. It provides good I/O performance as well as robust and flexible parallel I/O capabilities and may to replace others file formats once it will be complete. It can be read/written using either hdf5 directly or the vtkhdf implementation in VTK.

For more details, read the [related documentation](vtkhdf_file_format/index.md).
