# VTKHDF: Support reading and writing composite distributed temporal datasets

The vtkHDFWriter support now extends to composite temporal datasets (multiblock and partitioned dataset collection), all written in a same file.
Composite temporal datasets can also be written and read efficiently in parallel using MPI, and support static meshes.

Support for reading composite structures containing ImageData has been fixed. `vtkHDFReader` protected members `WholeExtent`, `Origin` and `Spacing` have been deleted.
