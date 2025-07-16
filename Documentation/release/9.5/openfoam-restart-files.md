# OpenFOAM reader: allow `_0` files

The OpenFOAM file reader in VTK now provides an option to
load file names which ends with `_0`. The loading was disabled
because OpenFOAM restart files usually ends with `_0`, but it prevented
loading of actual result files with the same naming schema.
