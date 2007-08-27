This directory contains a subset of the NetCDF source code.
Specifically, it is the "libsrc" directory of NetCDF, since
VTK only requires the NetCDF library itself and not any
utilities built on top of it.

The only differences to the vendor's release are:
- The README (as README.netcdf), RELEASE_NOTES, and COPYRIGHT
  files from the top level of the NetCDF source code are also
  included.
- Tabs aren't allowed by VTK's CVS checkin scripts, so they
  have been converted to spaces.

The trunk of the branch corresponds to the NetCDF 3.6.2 release.
