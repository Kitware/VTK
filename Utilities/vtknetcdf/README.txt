This directory contains a subset of the NetCDF source code.
Specifically, it is the source of the library itself
and not any utilities built on top of it.

The only differences to the vendor's release are:
- The README (as README.netcdf), RELEASE_NOTES, and COPYRIGHT
  files from the top level of the NetCDF source code are also
  included.
- Tabs aren't allowed by VTK's checkin scripts, so they
  have been converted to spaces.
- some "index" parameters have been renamed to "index2" to avoid
  shadowing "index" BSD function.


The head of the netcdf-release branch corresponds to the
NetCDF 4.1.2 release.

Instructions for updating to a new NetCDF release:
0. Go to a place where you can have some working directories:
   cd /tmp

1. Check out the netcdf-release branch

   cvs -d :pserver:anonymous@www.vtk.org:/cvsroot/VTK -z3 \
     co -r netcdf-release VTK/Utilities/vtknetcdf

2. Download and unpack the latest netcdf release:

   wget ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf.tar.gz
   tar xzvf netcdf.tar.gz

3. Copy files from the netCDF distribution into the VTK
   NetCDF checkout and commit:

   cp netcdf-*/libsrc/* vtknetcdf
   cp netcdf-*/README vtknetcdf/README.netcdf
   cp netcdf-*/RELEASE_NOTES netcdf-*/COPYRIGHT vtknetcdf
   cd vtknetcdf
   cvs commit -m "ENH: A

   You will need to change tabs to spaces before committing.
   You may need to cvs add or cvs remove files to track the
   NetCDF distribution.

4. Now merge the changes into the trunk:

   cvs update -PAd
   cvs update -j netcdf-release

5. Take care of any conflicts.

6. Build, test, and commit

7. Listen to the dashboards for problems on other compilers/architectures.
