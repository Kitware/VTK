# ADIOS2 VTX reader changes

## Fix cell data with time in ADIOS2 VTX reader

The ADIOS2 VTX reader supports cell data. However, the previous implementation of the reader
hard-coded the cell data to be static so that only the first time step was read. The new
implementation checks the time dimension for cell (and point) fields and will automatically read a
field over time if available.

## Fixed ADIOS2 VTX reader error reporting

The ADIOS2 VTX reader previously reported errors by throwing exceptions.  This causes lots of
problems with VTK and programs that use it such as ParaView because exceptions are not expected.
They are not caught or recovered from. Instead, it just causes applications to crash without
warning. The ADIOS2 VTX reader now properly reports errors by calling `vtkErrorMacro` and returning
0 from its process request.

## Add protection for missing arrays in ADIOS2 VTX reader

Previously, when the ADIOS2 VTX reader read in most data arrays from the ADIOS2 file, it would
silently leave a null array if that array did not exist. This opened up the likely consequence of
the program later crashing when the reader attempted to use this array.

Instead, the reader now reports an error when an array it attempts to read in is missing. This
prevents subsequent problems.

## IOADIOS2 VTX reader supports non-MPI builds

The ADIOS2 VTX reader can now be built with or without MPI enabled.
