/* Copyright 2010 University Corporation for Atmospheric
   Research/Unidata. See COPYRIGHT file for more info.

   This file has the strerror function.

   "$Id: copy.c,v 1.1 2010/06/01 15:46:49 ed Exp $" 
*/

#include "ncdispatch.h"

/* Tell the user the version of netCDF. */
static const char nc_libvers[] = PACKAGE_VERSION " of "__DATE__" "__TIME__" $";

const char *
nc_inq_libvers(void)
{
   return nc_libvers;
}

/* Given an error number, return an error message. */
const char *
nc_strerror(int ncerr1)
{
   /* System error? */
   if(NC_ISSYSERR(ncerr1))
   {
      const char *cp = (const char *) strerror(ncerr1);
      if(cp == NULL)
	 return "Unknown Error";
      return cp;
   }

   /* If we're here, this is a netcdf error code. */
   switch(ncerr1)
   {
      case NC_NOERR:
	 return "No error";
      case NC_EBADID:
	 return "NetCDF: Not a valid ID";
      case NC_ENFILE:
	 return "NetCDF: Too many files open";
      case NC_EEXIST:
	 return "NetCDF: File exists && NC_NOCLOBBER";
      case NC_EINVAL:
	 return "NetCDF: Invalid argument";
      case NC_EPERM:
	 return "NetCDF: Write to read only";
      case NC_ENOTINDEFINE:
	 return "NetCDF: Operation not allowed in data mode";
      case NC_EINDEFINE:
	 return "NetCDF: Operation not allowed in define mode";
      case NC_EINVALCOORDS:
	 return "NetCDF: Index exceeds dimension bound";
      case NC_EMAXDIMS:
	 return "NetCDF: NC_MAX_DIMS exceeded";
      case NC_ENAMEINUSE:
	 return "NetCDF: String match to name in use";
      case NC_ENOTATT:
	 return "NetCDF: Attribute not found";
      case NC_EMAXATTS:
	 return "NetCDF: NC_MAX_ATTRS exceeded";
      case NC_EBADTYPE:
	 return "NetCDF: Not a valid data type or _FillValue type mismatch";
      case NC_EBADDIM:
	 return "NetCDF: Invalid dimension ID or name";
      case NC_EUNLIMPOS:
	 return "NetCDF: NC_UNLIMITED in the wrong index";
      case NC_EMAXVARS:
	 return "NetCDF: NC_MAX_VARS exceeded";
      case NC_ENOTVAR:
	 return "NetCDF: Variable not found";
      case NC_EGLOBAL:
	 return "NetCDF: Action prohibited on NC_GLOBAL varid";
      case NC_ENOTNC:
	 return "NetCDF: Unknown file format";
      case NC_ESTS:
	 return "NetCDF: In Fortran, string too short";
      case NC_EMAXNAME:
	 return "NetCDF: NC_MAX_NAME exceeded";
      case NC_EUNLIMIT:
	 return "NetCDF: NC_UNLIMITED size already in use";
      case NC_ENORECVARS:
	 return "NetCDF: nc_rec op when there are no record vars";
      case NC_ECHAR:
	 return "NetCDF: Attempt to convert between text & numbers";
      case NC_EEDGE:
	 return "NetCDF: Start+count exceeds dimension bound";
      case NC_ESTRIDE:
	 return "NetCDF: Illegal stride";
      case NC_EBADNAME:
	 return "NetCDF: Name contains illegal characters";
      case NC_ERANGE:
	 return "NetCDF: Numeric conversion not representable";
      case NC_ENOMEM:
	 return "NetCDF: Memory allocation (malloc) failure";
      case NC_EVARSIZE:
	 return "NetCDF: One or more variable sizes violate format constraints";
      case NC_EDIMSIZE:
	 return "NetCDF: Invalid dimension size";
      case NC_ETRUNC:
	 return "NetCDF: File likely truncated or possibly corrupted";
      case NC_EAXISTYPE:
	 return "NetCDF: Illegal axis type";
      case NC_EDAP:
	 return "NetCDF: DAP failure";
      case NC_ECURL:
	 return "NetCDF: libcurl failure";
      case NC_EIO:
	 return "NetCDF: I/O failure";
      case NC_ENODATA:
	 return "NetCDF: Variable has no data in DAP request";
      case NC_EDAPSVC:
	 return "NetCDF: DAP server error";
      case NC_EDAS:
	 return "NetCDF: Malformed or inaccessible DAP DAS";
      case NC_EDDS:
	 return "NetCDF: Malformed or inaccessible DAP DDS";
      case NC_EDATADDS:
	 return "NetCDF: Malformed or inaccessible DAP DATADDS";
      case NC_EDAPURL:
	 return "NetCDF: Malformed URL";
      case NC_EDAPCONSTRAINT:
	 return "NetCDF: Malformed Constraint";
      case NC_EHDFERR:
	 return "NetCDF: HDF error";
      case NC_ECANTREAD:
	 return "NetCDF: Can't read file";
      case NC_ECANTWRITE:
	 return "NetCDF: Can't write file";
      case NC_ECANTCREATE:
	 return "NetCDF: Can't create file";
      case NC_EFILEMETA:
	 return "NetCDF: Can't add HDF5 file metadata";
      case NC_EDIMMETA:      
	 return "NetCDF: Can't define dimensional metadata";
      case NC_EATTMETA:
	 return "NetCDF: Can't open HDF5 attribute";
      case NC_EVARMETA:
	 return "NetCDF: Problem with variable metadata.";
      case NC_ENOCOMPOUND:
	 return "NetCDF: Can't create HDF5 compound type";
      case NC_EATTEXISTS:
	 return "NetCDF: Attempt to create attribute that alread exists";
      case NC_ENOTNC4:
	 return "NetCDF: Attempting netcdf-4 operation on netcdf-3 file";
      case NC_ESTRICTNC3:
	 return "NetCDF: Attempting netcdf-4 operation on strict nc3 netcdf-4 file";
      case NC_ENOTNC3:
	 return "NetCDF: Attempting netcdf-3 operation on netcdf-4 file";
      case NC_ENOPAR:
	 return "NetCDF: Parallel operation on file opened for non-parallel access";
      case NC_EPARINIT:
	 return "NetCDF: Error initializing for parallel access";
      case NC_EBADGRPID:
	 return "NetCDF: Bad group ID";
      case NC_EBADTYPID:
	 return "NetCDF: Bad type ID";
      case NC_ETYPDEFINED:
	 return "NetCDF: Type has already been defined and may not be edited";
      case NC_EBADFIELD:
	 return "NetCDF: Bad field ID";
      case NC_EBADCLASS:
	 return "NetCDF: Bad class";
      case NC_EMAPTYPE:
	 return "NetCDF: Mapped access for atomic types only";
      case NC_ELATEFILL:
	 return "NetCDF: Attempt to define fill value when data already exists.";
      case NC_ELATEDEF:
	 return "NetCDF: Attempt to define var properties, like deflate, after enddef.";
      case NC_EDIMSCALE:
	 return "NetCDF: Probem with HDF5 dimscales.";
      case NC_ENOGRP:
	 return "NetCDF: No group found.";
      case NC_ESTORAGE:
	 return "NetCDF: Cannot specify both contiguous and chunking.";
      case NC_EBADCHUNK:
	 return "NetCDF: Bad chunk sizes.";
      case NC_ENOTBUILT:
	 return "NetCDF: Attempt to use feature that was not turned on "
	    "when netCDF was built.";
      default:
	 return "Unknown Error";
   }
}


