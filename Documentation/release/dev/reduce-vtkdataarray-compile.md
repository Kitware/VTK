## Split DataArray type instantiations across more files

The vtkSOADataArrayTemplate instantiations and the array range
computations are now split by type across multiple cxx files.
Since fewer instantiations are done per file, the compiler
requires much less memory per file, and builds can be done on
systems with less RAM.
