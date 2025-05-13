# Add vtkFidesWriter for writing out data in ADIOS2 BP format

This adds `vtkFidesWriter` which uses the Fides library to write data to ADIOS2 BP files.
This initial version only provides support for the BP engines (SST support is in progress).
When writing the data, Fides will also write the schema as an attribute in the BP file,
so the data can be read back in with `vtkFidesReader`.
